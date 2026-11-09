#include "ll1_parser.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace minidb;

Token makeToken(const std::string& lex, TokenType type = TokenType::kIdentifier, std::size_t l = 1, std::size_t c = 1) {
    Token t;
    t.lexeme = lex;
    t.type = type;
    t.line = l;
    t.column = c;
    return t;
}

void runTest(const std::string& name, const std::vector<Token>& tokens, bool expected) {
    std::cout << "\n=========================================================================\n";
    std::cout << "=== TEST: " << name << " ===" << std::endl;

    LL1Parser parser;
    bool result = parser.parse(tokens);

    if (result == expected) {
        std::cout << "\n>>> [PASS] Test superado." << std::endl;
    } else {
        std::cout << "\n>>> [FAIL] Resultado incorrecto. Esperado: " << (expected ? "True" : "False") 
                  << ", Obtenido: " << (result ? "True" : "False") << std::endl;
    }
}

int main() {
    // 1. SELECT * FROM usuarios ;
    runTest("Consulta Valida (Proyeccion Simple)",
            {
                makeToken("SELECT", TokenType::kSelect),
                makeToken("*", TokenType::kStar),
                makeToken("FROM", TokenType::kFrom),
                makeToken("usuarios", TokenType::kIdentifier),
                makeToken(";", TokenType::kSemicolon)
            }, true);

    // 2. SELECT id , nombre FROM empleados WHERE edad >= 18 AND salario < 5000 ;
    runTest("Consulta Valida (Multiples Columnas + WHERE Condicional)",
            {
                makeToken("SELECT", TokenType::kSelect),
                makeToken("id", TokenType::kIdentifier),
                makeToken(",", TokenType::kComma),
                makeToken("nombre", TokenType::kIdentifier),
                makeToken("FROM", TokenType::kFrom),
                makeToken("empleados", TokenType::kIdentifier),
                makeToken("WHERE", TokenType::kWhere),
                makeToken("edad", TokenType::kIdentifier),
                makeToken(">=", TokenType::kGreaterEqual),
                makeToken("18", TokenType::kIntegerLiteral),
                makeToken("AND", TokenType::kAnd),
                makeToken("salario", TokenType::kIdentifier),
                makeToken("<", TokenType::kLess),
                makeToken("5000", TokenType::kIntegerLiteral),
                makeToken(";", TokenType::kSemicolon)
            }, true);

    // 3. Sintaxis Invalida: SELECT * FROM tabla (Falta ;)
    runTest("Sintaxis Invalida (Falta ';')",
            {
                makeToken("SELECT", TokenType::kSelect),
                makeToken("*", TokenType::kStar),
                makeToken("FROM", TokenType::kFrom),
                makeToken("tabla", TokenType::kIdentifier)
            }, false);

    // 4. Sintaxis Invalida: SELECT col FROM tabla WHERE ;
    runTest("Sintaxis Invalida (WHERE sin condicion)",
            {
                makeToken("SELECT", TokenType::kSelect),
                makeToken("col", TokenType::kIdentifier),
                makeToken("FROM", TokenType::kFrom),
                makeToken("tabla", TokenType::kIdentifier),
                makeToken("WHERE", TokenType::kWhere),
                makeToken(";", TokenType::kSemicolon)
            }, false);

    return 0;
}