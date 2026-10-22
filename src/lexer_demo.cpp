// Demo del analizador léxico.
//   lexer_demo                 lee la sentencia desde la entrada estándar
//   lexer_demo archivo.sql     lee la sentencia desde un archivo
//   lexer_demo -e "SELECT 1"   usa el texto dado
// Código de salida: 0 sin errores léxicos, 1 con errores, 2 si falla la lectura.
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "minidb/lexer/lexer.hpp"

int main(int argc, char** argv) {
    std::string input;
    if (argc == 3 && std::string(argv[1]) == "-e") {
        input = argv[2];
    } else if (argc == 2) {
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) {
            std::cerr << "No se pudo abrir '" << argv[1] << "'\n";
            return 2;
        }
        input.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    } else if (argc == 1) {
        input.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    } else {
        std::cerr << "Uso: lexer_demo [archivo.sql | -e \"sentencia\"]\n";
        return 2;
    }

    const auto result = minidb::lexer::Tokenize(input);
    minidb::lexer::PrintResult(std::cout, result);
    return result.Ok() ? 0 : 1;
}
