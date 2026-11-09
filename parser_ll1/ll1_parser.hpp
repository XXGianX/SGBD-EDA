#ifndef MINIDB_LL1_PARSER_HPP
#define MINIDB_LL1_PARSER_HPP

#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <string>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

#include "minidb/lexer/token.hpp"

namespace minidb {

// Alias para usar Token directamente del namespace del lexer
using Token = minidb::lexer::Token;
using TokenType = minidb::lexer::TokenType;

enum class Terminal {
    SELECT, FROM, WHERE, AND, OR, IDENTIFIER, NUMBER, STAR, COMMA,
    SEMICOLON, EQ, NEQ, LT, GT, LE, GE, END_OF_FILE, EPSILON, INVALID
};

enum class NonTerminal {
    STATEMENT, SELECT_LIST, COLUMN_LIST_TAIL, WHERE_CLAUSE,
    CONDITION, CONDITION_TAIL, EXPRESSION, REL_OP, VALUE
};

struct Symbol {
    bool is_terminal;
    Terminal term;
    NonTerminal non_term;

    static Symbol createTerminal(Terminal t) { return {true, t, NonTerminal::STATEMENT}; }
    static Symbol createNonTerminal(NonTerminal nt) { return {false, Terminal::INVALID, nt}; }
};

class LL1Parser {
public:
    LL1Parser();

    bool parse(const std::vector<Token>& tokens);

private:
    std::map<NonTerminal, std::map<Terminal, std::vector<Symbol>>> parse_table_;
    std::map<Terminal, std::string> term_names_;
    std::map<NonTerminal, std::string> non_term_names_;

    void initNames();
    void buildParseTable();

    Terminal mapTokenToTerminal(const Token& token);
    std::string getSymbolName(const Symbol& sym);
};

} // namespace minidb
#endif