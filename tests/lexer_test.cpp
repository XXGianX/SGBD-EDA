#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "minidb/lexer/lexer.hpp"

namespace minidb::lexer {
namespace {

using T = TokenType;

std::vector<TokenType> Types(const LexResult& r) {
    std::vector<TokenType> out;
    for (const Token& t : r.tokens) out.push_back(t.type);
    return out;
}

std::vector<std::string> Lexemes(const LexResult& r) {
    std::vector<std::string> out;
    for (const Token& t : r.tokens) out.push_back(t.lexeme);
    return out;
}

// --- 1. Sentencia válida -------------------------------------------------------

TEST(LexerValid, EnunciadoExample) {
    const auto r = Tokenize("SELECT nombre, edad\nFROM estudiantes\nWHERE edad >= 18;");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kSelect, T::kIdentifier, T::kComma,
                                                T::kIdentifier, T::kFrom, T::kIdentifier,
                                                T::kWhere, T::kIdentifier, T::kGreaterEqual,
                                                T::kIntegerLiteral, T::kSemicolon,
                                                T::kEndOfFile}));
    EXPECT_EQ(Lexemes(r), (std::vector<std::string>{"SELECT", "nombre", ",", "edad", "FROM",
                                                    "estudiantes", "WHERE", "edad", ">=", "18",
                                                    ";", ""}));
}

TEST(LexerValid, AndOrAndStarAndParens) {
    const auto r = Tokenize("SELECT * FROM t WHERE (a = 1 AND b = 2) OR c <> 3");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{
                            T::kSelect, T::kStar, T::kFrom, T::kIdentifier, T::kWhere,
                            T::kLeftParen, T::kIdentifier, T::kEqual, T::kIntegerLiteral, T::kAnd,
                            T::kIdentifier, T::kEqual, T::kIntegerLiteral, T::kRightParen, T::kOr,
                            T::kIdentifier, T::kNotEqual, T::kIntegerLiteral, T::kEndOfFile}));
}

// --- 2. Operadores compuestos ---------------------------------------------------

TEST(LexerOperators, CompoundOperatorsAreSingleTokens) {
    const auto r = Tokenize(">= <= <> != = < >");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kGreaterEqual, T::kLessEqual, T::kNotEqual,
                                                T::kNotEqual, T::kEqual, T::kLess, T::kGreater,
                                                T::kEndOfFile}));
    EXPECT_EQ(Lexemes(r)[2], "<>");
    EXPECT_EQ(Lexemes(r)[3], "!=");
}

TEST(LexerOperators, NoSpacesBetweenOperatorAndOperands) {
    const auto r = Tokenize("a>=1 b<=2 c<>3 d!=4");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r)[1], T::kGreaterEqual);
    EXPECT_EQ(Types(r)[4], T::kLessEqual);
    EXPECT_EQ(Types(r)[7], T::kNotEqual);
    EXPECT_EQ(Types(r)[10], T::kNotEqual);
}

TEST(LexerOperators, SeparatedOperatorsAreNotMerged) {
    // '>' y '=' separados por un espacio son dos tokens, no un '>='.
    const auto r = Tokenize("> =");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kGreater, T::kEqual, T::kEndOfFile}));
}

// --- 3. Literales ---------------------------------------------------------------

TEST(LexerLiterals, IntegerAndDecimal) {
    const auto r = Tokenize("0 42 3.14 0.5 007");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kIntegerLiteral, T::kIntegerLiteral,
                                                T::kDecimalLiteral, T::kDecimalLiteral,
                                                T::kIntegerLiteral, T::kEndOfFile}));
    EXPECT_EQ(r.tokens[2].lexeme, "3.14");
}

TEST(LexerLiterals, StringWithoutQuotesInLexeme) {
    const auto r = Tokenize("'Ana Perez'");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[0].type, T::kStringLiteral);
    EXPECT_EQ(r.tokens[0].lexeme, "Ana Perez");
}

TEST(LexerLiterals, DoubledQuoteIsLiteralQuote) {
    const auto r = Tokenize("'O''Brien'");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[0].lexeme, "O'Brien");
}

TEST(LexerLiterals, EmptyString) {
    const auto r = Tokenize("''");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[0].type, T::kStringLiteral);
    EXPECT_EQ(r.tokens[0].lexeme, "");
}

TEST(LexerLiterals, StringKeepsKeywordsOperatorsAndUtf8Inside) {
    const auto r = Tokenize("'SELECT >= ; Computación'");
    ASSERT_TRUE(r.Ok());
    ASSERT_EQ(r.tokens.size(), 2U);
    EXPECT_EQ(r.tokens[0].lexeme, "SELECT >= ; Computación");
}

// --- 4. Palabras reservadas vs identificadores ----------------------------------

TEST(LexerKeywords, CaseInsensitiveAndOriginalSpellingKept) {
    const auto r = Tokenize("select From wHeRe and OR");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kSelect, T::kFrom, T::kWhere, T::kAnd, T::kOr,
                                                T::kEndOfFile}));
    EXPECT_EQ(r.tokens[0].lexeme, "select");
}

TEST(LexerKeywords, PrefixOrSuffixOfKeywordIsIdentifier) {
    const auto r = Tokenize("selected fromage orange android _select select1");
    ASSERT_TRUE(r.Ok());
    for (std::size_t i = 0; i + 1 < r.tokens.size(); ++i) {
        EXPECT_EQ(r.tokens[i].type, T::kIdentifier) << r.tokens[i].lexeme;
    }
}

TEST(LexerKeywords, IdentifiersWithDigitsAndUnderscore) {
    const auto r = Tokenize("tabla_1 _x a1b2");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Lexemes(r), (std::vector<std::string>{"tabla_1", "_x", "a1b2", ""}));
}

// --- 5. Espacios en blanco y posiciones -------------------------------------------

TEST(LexerWhitespace, TabsNewlinesAndCrLfAreIgnored) {
    const auto r = Tokenize("SELECT\t a\r\n,\n\n b \t ");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kSelect, T::kIdentifier, T::kComma,
                                                T::kIdentifier, T::kEndOfFile}));
}

TEST(LexerPositions, LineAndColumnOfEveryToken) {
    const auto r = Tokenize("SELECT a\n  FROM t");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[0].line, 1U);
    EXPECT_EQ(r.tokens[0].column, 1U);
    EXPECT_EQ(r.tokens[1].line, 1U);
    EXPECT_EQ(r.tokens[1].column, 8U);
    EXPECT_EQ(r.tokens[2].line, 2U);
    EXPECT_EQ(r.tokens[2].column, 3U);
    EXPECT_EQ(r.tokens[3].line, 2U);
    EXPECT_EQ(r.tokens[3].column, 8U);
    EXPECT_EQ(r.tokens[4].type, T::kEndOfFile);
    EXPECT_EQ(r.tokens[4].line, 2U);
    EXPECT_EQ(r.tokens[4].column, 9U);
}

TEST(LexerPositions, ColumnCountsUtf8CharactersNotBytes) {
    const auto r = Tokenize("'ñ' x");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[1].column, 5U);
}

TEST(LexerPositions, MultiLineStringAdvancesLine) {
    const auto r = Tokenize("'a\nb' x");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(r.tokens[1].line, 2U);
    EXPECT_EQ(r.tokens[1].column, 4U);
}

// --- 6. Errores léxicos ----------------------------------------------------------

TEST(LexerErrors, InvalidCharacterReportedWithPosition) {
    const auto r = Tokenize("SELECT a\nFROM t WHERE a @ 1");
    ASSERT_EQ(r.errors.size(), 1U);
    EXPECT_EQ(r.errors[0].kind, LexErrorKind::kInvalidCharacter);
    EXPECT_EQ(r.errors[0].lexeme, "@");
    EXPECT_EQ(r.errors[0].line, 2U);
    EXPECT_EQ(r.errors[0].column, 16U);
}

TEST(LexerErrors, ErrorsAreNeverTokensAndScanningContinues) {
    const auto r = Tokenize("a # b $ c");
    EXPECT_EQ(r.errors.size(), 2U);
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kIdentifier, T::kIdentifier, T::kIdentifier,
                                                T::kEndOfFile}));
}

TEST(LexerErrors, NonAsciiOutsideStringIsOneError) {
    const auto r = Tokenize("a € b");
    ASSERT_EQ(r.errors.size(), 1U);
    EXPECT_EQ(r.errors[0].lexeme, "€");
    EXPECT_EQ(r.errors[0].column, 3U);
    EXPECT_EQ(r.tokens.size(), 3U);  // a, b, EOF
    EXPECT_EQ(r.tokens[1].column, 5U);
}

TEST(LexerErrors, UnterminatedString) {
    const auto r = Tokenize("WHERE nombre = 'Ana");
    ASSERT_EQ(r.errors.size(), 1U);
    EXPECT_EQ(r.errors[0].kind, LexErrorKind::kUnterminatedString);
    EXPECT_EQ(r.errors[0].line, 1U);
    EXPECT_EQ(r.errors[0].column, 16U);
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kWhere, T::kIdentifier, T::kEqual,
                                                T::kEndOfFile}));
}

TEST(LexerErrors, StringEndingInEscapedQuoteIsUnterminated) {
    const auto r = Tokenize("'abc''");
    ASSERT_EQ(r.errors.size(), 1U);
    EXPECT_EQ(r.errors[0].kind, LexErrorKind::kUnterminatedString);
}

TEST(LexerErrors, MalformedNumbers) {
    for (const char* bad : {"12.", "1.2.3", "12abc", "3.x", "1_000"}) {
        const auto r = Tokenize(bad);
        ASSERT_EQ(r.errors.size(), 1U) << bad;
        EXPECT_EQ(r.errors[0].kind, LexErrorKind::kMalformedNumber) << bad;
        EXPECT_EQ(r.errors[0].lexeme, bad);
        EXPECT_EQ(r.tokens.size(), 1U) << bad;  // solo EOF
    }
}

TEST(LexerErrors, LoneBangIsInvalidOperator) {
    const auto r = Tokenize("a ! b");
    ASSERT_EQ(r.errors.size(), 1U);
    EXPECT_EQ(r.errors[0].kind, LexErrorKind::kInvalidOperator);
    EXPECT_EQ(r.errors[0].column, 3U);
}

TEST(LexerErrors, MinusAndDotAreNotInTheLanguage) {
    EXPECT_EQ(Tokenize("a - 1").errors.size(), 1U);
    EXPECT_EQ(Tokenize("t.a").errors.size(), 1U);
}

TEST(LexerErrors, MultipleErrorsAllReportedInOrder) {
    const auto r = Tokenize("@ 'x\n");
    ASSERT_EQ(r.errors.size(), 2U);
    EXPECT_EQ(r.errors[0].kind, LexErrorKind::kInvalidCharacter);
    EXPECT_EQ(r.errors[1].kind, LexErrorKind::kUnterminatedString);
}

// --- 7. Casos límite ----------------------------------------------------------------

TEST(LexerEdgeCases, EmptyInputIsOnlyEof) {
    const auto r = Tokenize("");
    ASSERT_TRUE(r.Ok());
    ASSERT_EQ(r.tokens.size(), 1U);
    EXPECT_EQ(r.tokens[0].type, T::kEndOfFile);
    EXPECT_EQ(r.tokens[0].line, 1U);
    EXPECT_EQ(r.tokens[0].column, 1U);
}

TEST(LexerEdgeCases, OnlyWhitespaceIsOnlyEof) {
    const auto r = Tokenize(" \t\r\n\n  ");
    ASSERT_TRUE(r.Ok());
    ASSERT_EQ(r.tokens.size(), 1U);
    EXPECT_EQ(r.tokens[0].line, 3U);
    EXPECT_EQ(r.tokens[0].column, 3U);
}

TEST(LexerEdgeCases, IncompleteStatementStillTokenizes) {
    // El lexer no valida la gramática: eso es trabajo del parser.
    const auto r = Tokenize("SELECT FROM WHERE");
    ASSERT_TRUE(r.Ok());
    EXPECT_EQ(Types(r), (std::vector<TokenType>{T::kSelect, T::kFrom, T::kWhere, T::kEndOfFile}));
}

TEST(LexerEdgeCases, TokenTouchingEndOfInput) {
    EXPECT_EQ(Tokenize("x").tokens[0].lexeme, "x");
    EXPECT_EQ(Tokenize("42").tokens[0].type, T::kIntegerLiteral);
    EXPECT_EQ(Tokenize("<").tokens[0].type, T::kLess);
    EXPECT_EQ(Tokenize(">=").tokens[0].type, T::kGreaterEqual);
    EXPECT_EQ(Tokenize(";").tokens[0].type, T::kSemicolon);
    EXPECT_EQ(Tokenize("'a'").tokens[0].type, T::kStringLiteral);
}

TEST(LexerEdgeCases, EofIsAlwaysLastAndUnique) {
    for (const char* sql : {"", "SELECT", "@@", "'x", "a b c"}) {
        const auto r = Tokenize(sql);
        ASSERT_FALSE(r.tokens.empty());
        EXPECT_EQ(r.tokens.back().type, T::kEndOfFile) << sql;
        for (std::size_t i = 0; i + 1 < r.tokens.size(); ++i) {
            EXPECT_NE(r.tokens[i].type, T::kEndOfFile) << sql;
        }
    }
}

TEST(LexerEdgeCases, LexerIsReusableAcrossInstances) {
    EXPECT_EQ(Tokenize("a").tokens.size(), Tokenize("a").tokens.size());
    Lexer lexer("a b");
    EXPECT_EQ(lexer.Tokenize().tokens.size(), 3U);
}

// --- Interfaz de salida ----------------------------------------------------------------

TEST(LexerOutput, TokenTypeNamesFollowEnunciadoTable) {
    EXPECT_EQ(TokenTypeName(T::kSelect), "KEYWORD_SELECT");
    EXPECT_EQ(TokenTypeName(T::kGreaterEqual), "GREATER_EQUAL");
    EXPECT_EQ(TokenTypeName(T::kIntegerLiteral), "INTEGER_LITERAL");
    EXPECT_EQ(TokenTypeName(T::kEndOfFile), "EOF");
}

TEST(LexerOutput, PrintResultListsTokensThenErrors) {
    std::ostringstream out;
    PrintResult(out, Tokenize("a @"));
    const std::string text = out.str();
    EXPECT_NE(text.find("IDENTIFIER"), std::string::npos);
    EXPECT_NE(text.find("EOF"), std::string::npos);
    EXPECT_NE(text.find("Error léxico [INVALID_CHARACTER] en 1:3"), std::string::npos);
}

}  // namespace
}  // namespace minidb::lexer
