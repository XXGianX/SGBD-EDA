#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace minidb::lexer {

/// Categorías léxicas. Los nombres impresos (TokenTypeName) siguen la tabla del
/// enunciado: KEYWORD_SELECT, IDENTIFIER, INTEGER_LITERAL, GREATER_EQUAL, ...
enum class TokenType : std::uint8_t {
    // Palabras reservadas
    kSelect,
    kFrom,
    kWhere,
    kAnd,
    kOr,
    // Identificadores y literales
    kIdentifier,
    kIntegerLiteral,
    kDecimalLiteral,
    kStringLiteral,
    // Operadores relacionales (`!=` y `<>` producen el mismo tipo)
    kEqual,
    kNotEqual,
    kLess,
    kGreater,
    kLessEqual,
    kGreaterEqual,
    // Puntuación
    kComma,
    kSemicolon,
    kStar,
    kLeftParen,
    kRightParen,
    // Fin de entrada
    kEndOfFile,
};

[[nodiscard]] std::string_view TokenTypeName(TokenType type);

/// Un token con su posición de inicio (línea y columna, base 1; la columna cuenta
/// caracteres UTF-8, no bytes).
///
/// `lexeme` es el texto tal como aparece en la entrada, salvo en kStringLiteral,
/// donde es el contenido sin las comillas y con `''` ya convertido en `'`.
struct Token {
    TokenType type = TokenType::kEndOfFile;
    std::string lexeme;
    std::size_t line = 1;
    std::size_t column = 1;

    bool operator==(const Token&) const = default;
};

enum class LexErrorKind : std::uint8_t {
    kInvalidCharacter,   // carácter fuera del alfabeto del lenguaje
    kUnterminatedString, // comilla simple sin cerrar
    kMalformedNumber,    // 12.  1.2.3  12abc
    kInvalidOperator,    // '!' que no forma parte de '!='
};

[[nodiscard]] std::string_view LexErrorKindName(LexErrorKind kind);

struct LexError {
    LexErrorKind kind = LexErrorKind::kInvalidCharacter;
    std::string message;
    std::string lexeme;  // texto ofensivo, tal como aparece en la entrada
    std::size_t line = 1;
    std::size_t column = 1;
};

/// Resultado del análisis. Los errores nunca aparecen en `tokens`: una secuencia
/// mal formada se informa y se descarta. `tokens` siempre termina en kEndOfFile.
struct LexResult {
    std::vector<Token> tokens;
    std::vector<LexError> errors;

    [[nodiscard]] bool Ok() const { return errors.empty(); }
};

}  // namespace minidb::lexer
