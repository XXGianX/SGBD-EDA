#include "minidb/lexer/lexer.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

namespace minidb::lexer {
namespace {

bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool IsDigit(char c) { return c >= '0' && c <= '9'; }
bool IsAlnum(char c) { return IsAlpha(c) || IsDigit(c); }
bool IsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

char ToUpperAscii(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }

/// Tabla de palabras reservadas. Se compara en mayúsculas: las palabras clave no
/// distinguen mayúsculas de minúsculas.
struct Keyword {
    std::string_view text;
    TokenType type;
};
constexpr std::array<Keyword, 5> kKeywords{{
    {"SELECT", TokenType::kSelect},
    {"FROM", TokenType::kFrom},
    {"WHERE", TokenType::kWhere},
    {"AND", TokenType::kAnd},
    {"OR", TokenType::kOr},
}};

/// Un lexema numérico válido es dígitos+ o dígitos+ '.' dígitos+.
/// Devuelve true si es decimal, false si es entero; `valid` indica si está bien formado.
bool ClassifyNumber(std::string_view text, bool& valid) {
    const auto dot = text.find('.');
    const auto all_digits = [](std::string_view s) {
        return !s.empty() && std::ranges::all_of(s, IsDigit);
    };
    if (dot == std::string_view::npos) {
        valid = all_digits(text);
        return false;
    }
    valid = all_digits(text.substr(0, dot)) && all_digits(text.substr(dot + 1));
    return true;
}

}  // namespace

std::string_view TokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::kSelect: return "KEYWORD_SELECT";
        case TokenType::kFrom: return "KEYWORD_FROM";
        case TokenType::kWhere: return "KEYWORD_WHERE";
        case TokenType::kAnd: return "KEYWORD_AND";
        case TokenType::kOr: return "KEYWORD_OR";
        case TokenType::kIdentifier: return "IDENTIFIER";
        case TokenType::kIntegerLiteral: return "INTEGER_LITERAL";
        case TokenType::kDecimalLiteral: return "DECIMAL_LITERAL";
        case TokenType::kStringLiteral: return "STRING_LITERAL";
        case TokenType::kEqual: return "EQUAL";
        case TokenType::kNotEqual: return "NOT_EQUAL";
        case TokenType::kLess: return "LESS";
        case TokenType::kGreater: return "GREATER";
        case TokenType::kLessEqual: return "LESS_EQUAL";
        case TokenType::kGreaterEqual: return "GREATER_EQUAL";
        case TokenType::kComma: return "COMMA";
        case TokenType::kSemicolon: return "SEMICOLON";
        case TokenType::kStar: return "STAR";
        case TokenType::kLeftParen: return "LEFT_PAREN";
        case TokenType::kRightParen: return "RIGHT_PAREN";
        case TokenType::kEndOfFile: return "EOF";
    }
    return "UNKNOWN";
}

std::string_view LexErrorKindName(LexErrorKind kind) {
    switch (kind) {
        case LexErrorKind::kInvalidCharacter: return "INVALID_CHARACTER";
        case LexErrorKind::kUnterminatedString: return "UNTERMINATED_STRING";
        case LexErrorKind::kMalformedNumber: return "MALFORMED_NUMBER";
        case LexErrorKind::kInvalidOperator: return "INVALID_OPERATOR";
    }
    return "UNKNOWN";
}

// --- Lexer ------------------------------------------------------------------

char Lexer::Peek(std::size_t offset) const {
    return pos_ + offset < source_.size() ? source_[pos_ + offset] : '\0';
}

char Lexer::Advance() {
    const char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else if ((static_cast<unsigned char>(c) & 0xC0U) != 0x80U) {
        // Los bytes de continuación UTF-8 (10xxxxxx) no avanzan la columna.
        ++column_;
    }
    return c;
}

bool Lexer::Match(char expected) {
    if (AtEnd() || source_[pos_] != expected) {
        return false;
    }
    Advance();
    return true;
}

void Lexer::SkipWhitespace() {
    while (!AtEnd() && IsSpace(source_[pos_])) {
        Advance();
    }
}

void Lexer::Emit(TokenType type, std::string lexeme) {
    result_.tokens.push_back(Token{type, std::move(lexeme), start_line_, start_column_});
}

void Lexer::Fail(LexErrorKind kind, std::string message, std::string lexeme) {
    result_.errors.push_back(
        LexError{kind, std::move(message), std::move(lexeme), start_line_, start_column_});
}

LexResult Lexer::Tokenize() {
    result_ = LexResult{};
    pos_ = 0;
    line_ = 1;
    column_ = 1;

    while (true) {
        SkipWhitespace();
        if (AtEnd()) {
            break;
        }
        start_pos_ = pos_;
        start_line_ = line_;
        start_column_ = column_;
        ScanToken();
    }

    start_line_ = line_;
    start_column_ = column_;
    Emit(TokenType::kEndOfFile, "");
    return std::move(result_);
}

void Lexer::ScanToken() {
    const char c = Peek();
    if (IsAlpha(c)) {
        ScanWord();
        return;
    }
    if (IsDigit(c)) {
        ScanNumber();
        return;
    }
    if (c == '\'') {
        ScanString();
        return;
    }

    Advance();
    switch (c) {
        case ',': Emit(TokenType::kComma, ","); return;
        case ';': Emit(TokenType::kSemicolon, ";"); return;
        case '*': Emit(TokenType::kStar, "*"); return;
        case '(': Emit(TokenType::kLeftParen, "("); return;
        case ')': Emit(TokenType::kRightParen, ")"); return;
        case '=': Emit(TokenType::kEqual, "="); return;
        case '<':
            // Maximal munch: se intenta primero el operador de dos caracteres.
            if (Match('=')) {
                Emit(TokenType::kLessEqual, "<=");
            } else if (Match('>')) {
                Emit(TokenType::kNotEqual, "<>");
            } else {
                Emit(TokenType::kLess, "<");
            }
            return;
        case '>':
            if (Match('=')) {
                Emit(TokenType::kGreaterEqual, ">=");
            } else {
                Emit(TokenType::kGreater, ">");
            }
            return;
        case '!':
            if (Match('=')) {
                Emit(TokenType::kNotEqual, "!=");
            } else {
                Fail(LexErrorKind::kInvalidOperator, "'!' solo es válido como parte de '!='",
                     "!");
            }
            return;
        default:
            // El primer byte ya fue consumido; ScanInvalid lo recupera con start_pos_.
            ScanInvalid();
            return;
    }
}

void Lexer::ScanWord() {
    while (IsAlnum(Peek())) {
        Advance();
    }
    const std::string_view text = source_.substr(start_pos_, pos_ - start_pos_);

    std::string upper(text);
    std::ranges::transform(upper, upper.begin(), ToUpperAscii);
    for (const Keyword& keyword : kKeywords) {
        if (keyword.text == upper) {
            Emit(keyword.type, std::string(text));  // conserva la grafía original
            return;
        }
    }
    Emit(TokenType::kIdentifier, std::string(text));
}

void Lexer::ScanNumber() {
    // Se consume la corrida completa [A-Za-z0-9_.] y luego se valida. Así
    // `12abc`, `1.2.3` y `12.` se informan como UN error y no como números
    // seguidos de identificadores.
    while (IsAlnum(Peek()) || Peek() == '.') {
        Advance();
    }
    const std::string_view text = source_.substr(start_pos_, pos_ - start_pos_);

    bool valid = false;
    const bool is_decimal = ClassifyNumber(text, valid);
    if (!valid) {
        Fail(LexErrorKind::kMalformedNumber, "literal numérico mal formado '" + std::string(text) + "'",
             std::string(text));
        return;
    }
    Emit(is_decimal ? TokenType::kDecimalLiteral : TokenType::kIntegerLiteral, std::string(text));
}

void Lexer::ScanString() {
    Advance();  // comilla de apertura
    std::string value;
    while (true) {
        if (AtEnd()) {
            Fail(LexErrorKind::kUnterminatedString, "cadena sin cerrar: falta la comilla simple final",
                 std::string(source_.substr(start_pos_)));
            return;
        }
        const char c = Advance();
        if (c == '\'') {
            if (Peek() == '\'') {  // '' dentro de la cadena = comilla literal
                Advance();
                value.push_back('\'');
                continue;
            }
            Emit(TokenType::kStringLiteral, std::move(value));
            return;
        }
        value.push_back(c);
    }
}

void Lexer::ScanInvalid() {
    // El primer byte ya fue consumido. Si es el inicio de una secuencia UTF-8
    // multibyte, se consumen sus bytes de continuación para informar un único
    // carácter y no basura de bytes sueltos.
    while ((static_cast<unsigned char>(Peek()) & 0xC0U) == 0x80U && !AtEnd()) {
        Advance();
    }
    const std::string text(source_.substr(start_pos_, pos_ - start_pos_));
    Fail(LexErrorKind::kInvalidCharacter, "carácter no admitido '" + text + "'", text);
}

LexResult Tokenize(std::string_view source) { return Lexer(source).Tokenize(); }

// --- Interfaz de salida -----------------------------------------------------

std::string FormatToken(const Token& token) {
    std::ostringstream out;
    out << std::left << std::setw(16) << TokenTypeName(token.type) << ' ' << std::setw(20)
        << ("'" + token.lexeme + "'") << ' ' << token.line << ':' << token.column;
    return out.str();
}

std::string FormatError(const LexError& error) {
    std::ostringstream out;
    out << "Error léxico [" << LexErrorKindName(error.kind) << "] en " << error.line << ':'
        << error.column << ": " << error.message;
    return out.str();
}

void PrintResult(std::ostream& out, const LexResult& result) {
    for (const Token& token : result.tokens) {
        out << FormatToken(token) << '\n';
    }
    for (const LexError& error : result.errors) {
        out << FormatError(error) << '\n';
    }
}

}  // namespace minidb::lexer
