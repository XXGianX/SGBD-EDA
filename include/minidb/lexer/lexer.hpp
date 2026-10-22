#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

#include "minidb/lexer/token.hpp"

namespace minidb::lexer {

/// Analizador léxico del subconjunto SQL. No ejecuta nada ni consulta el
/// catálogo: solo convierte texto en tokens. No depende de storage/, buffer/ ni
/// index/, así que el parser puede consumirlo de forma independiente.
class Lexer {
public:
    explicit Lexer(std::string_view source) : source_(source) {}

    /// Recorre toda la entrada. Ante un error léxico lo registra y continúa
    /// después de la secuencia ofensiva, para informar todos los errores de una vez.
    [[nodiscard]] LexResult Tokenize();

private:
    [[nodiscard]] bool AtEnd() const { return pos_ >= source_.size(); }
    [[nodiscard]] char Peek(std::size_t offset = 0) const;
    char Advance();
    bool Match(char expected);

    void SkipWhitespace();
    void ScanToken();
    void ScanWord();
    void ScanNumber();
    void ScanString();
    void ScanInvalid();

    void Emit(TokenType type, std::string lexeme);
    void Fail(LexErrorKind kind, std::string message, std::string lexeme);

    std::string_view source_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    // Posición donde empieza el lexema en curso.
    std::size_t start_pos_ = 0;
    std::size_t start_line_ = 1;
    std::size_t start_column_ = 1;
    LexResult result_;
};

/// Atajo: `Tokenize(sql)` equivale a `Lexer(sql).Tokenize()`.
[[nodiscard]] LexResult Tokenize(std::string_view source);

// --- Interfaz de salida -----------------------------------------------------

[[nodiscard]] std::string FormatToken(const Token& token);
[[nodiscard]] std::string FormatError(const LexError& error);

/// Imprime una tabla `TIPO  LEXEMA  línea:columna` seguida de los errores.
void PrintResult(std::ostream& out, const LexResult& result);

}  // namespace minidb::lexer
