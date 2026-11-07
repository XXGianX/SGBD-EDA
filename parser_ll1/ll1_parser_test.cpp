#include "ll1_parser.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace minidb;

Token makeToken(const std::string& lex) {
    Token t;
    t.lexeme = lex; 
    return t;
}

void runTest(const std::string& name, const std::vector<std::string>& token_strings, bool expected) {
    std::cout << "\n=========================================================================\n";
    std::cout << "=== TEST: " << name << " ===" << std::endl;
    std::vector<Token> tokens;
    for (const auto& s : token_strings) {
        tokens.push_back(makeToken(s));
    }

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
    runTest("Consulta Valida (Proyeccion Simple)",
            {"SELECT", "*", "FROM", "usuarios", ";"}, true);

    runTest("Consulta Valida (Multiples Columnas + WHERE Condicional)",
            {"SELECT", "id", ",", "nombre", "FROM", "empleados", "WHERE", "edad", ">=", "18", "AND", "salario", "<", "5000", ";"}, true);

    runTest("Sintaxis Invalida (Falta ';')",
            {"SELECT", "*", "FROM", "tabla"}, false);

    runTest("Sintaxis Invalida (WHERE sin condicion)",
            {"SELECT", "columna", "FROM", "tabla", "WHERE", ";"}, false);

    return 0;
}
