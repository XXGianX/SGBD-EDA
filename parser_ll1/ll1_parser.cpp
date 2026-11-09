#include "ll1_parser.hpp"

namespace minidb {

LL1Parser::LL1Parser() {
    initNames();
    buildParseTable();
}

void LL1Parser::initNames() {
    term_names_ = {
        {Terminal::SELECT, "SELECT"}, {Terminal::FROM, "FROM"},
        {Terminal::WHERE, "WHERE"}, {Terminal::AND, "AND"}, {Terminal::OR, "OR"},
        {Terminal::IDENTIFIER, "ID"}, {Terminal::NUMBER, "NUM"},
        {Terminal::STAR, "*"}, {Terminal::COMMA, ","}, {Terminal::SEMICOLON, ";"},
        {Terminal::EQ, "="}, {Terminal::NEQ, "<>"}, {Terminal::LT, "<"},
        {Terminal::GT, ">"}, {Terminal::LE, "<="}, {Terminal::GE, ">="},
        {Terminal::END_OF_FILE, "$"}, {Terminal::EPSILON, "EPSILON"}
    };
    non_term_names_ = {
        {NonTerminal::STATEMENT, "STATEMENT"},
        {NonTerminal::SELECT_LIST, "SELECT_LIST"},
        {NonTerminal::COLUMN_LIST_TAIL, "COLUMN_LIST_TAIL"},
        {NonTerminal::WHERE_CLAUSE, "WHERE_CLAUSE"},
        {NonTerminal::CONDITION, "CONDITION"},
        {NonTerminal::CONDITION_TAIL, "CONDITION_TAIL"},
        {NonTerminal::EXPRESSION, "EXPRESSION"},
        {NonTerminal::REL_OP, "REL_OP"},
        {NonTerminal::VALUE, "VALUE"}
    };
}

void LL1Parser::buildParseTable() {
    auto T = [](Terminal t) { return Symbol::createTerminal(t); };
    auto NT = [](NonTerminal nt) { return Symbol::createNonTerminal(nt); };

    // 1. STATEMENT -> SELECT SELECT_LIST FROM IDENTIFIER WHERE_CLAUSE ;
    parse_table_[NonTerminal::STATEMENT][Terminal::SELECT] = {
        T(Terminal::SELECT), NT(NonTerminal::SELECT_LIST), T(Terminal::FROM),
        T(Terminal::IDENTIFIER), NT(NonTerminal::WHERE_CLAUSE), T(Terminal::SEMICOLON)
    };

    // 2. SELECT_LIST -> *
    parse_table_[NonTerminal::SELECT_LIST][Terminal::STAR] = { T(Terminal::STAR) };
    // 3. SELECT_LIST -> IDENTIFIER COLUMN_LIST_TAIL
    parse_table_[NonTerminal::SELECT_LIST][Terminal::IDENTIFIER] = {
        T(Terminal::IDENTIFIER), NT(NonTerminal::COLUMN_LIST_TAIL)
    };

    // 4. COLUMN_LIST_TAIL -> , IDENTIFIER COLUMN_LIST_TAIL
    parse_table_[NonTerminal::COLUMN_LIST_TAIL][Terminal::COMMA] = {
        T(Terminal::COMMA), T(Terminal::IDENTIFIER), NT(NonTerminal::COLUMN_LIST_TAIL)
    };
    // 5. COLUMN_LIST_TAIL -> eps (FOLLOW es FROM)
    parse_table_[NonTerminal::COLUMN_LIST_TAIL][Terminal::FROM] = { T(Terminal::EPSILON) };

    // 6. WHERE_CLAUSE -> WHERE CONDITION
    parse_table_[NonTerminal::WHERE_CLAUSE][Terminal::WHERE] = {
        T(Terminal::WHERE), NT(NonTerminal::CONDITION)
    };
    // 7. WHERE_CLAUSE -> eps (FOLLOW es ;)
    parse_table_[NonTerminal::WHERE_CLAUSE][Terminal::SEMICOLON] = { T(Terminal::EPSILON) };

    // 8. CONDITION -> EXPRESSION CONDITION_TAIL
    parse_table_[NonTerminal::CONDITION][Terminal::IDENTIFIER] = {
        NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };

    // 9. CONDITION_TAIL -> AND EXPRESSION CONDITION_TAIL
    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::AND] = {
        T(Terminal::AND), NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };
    // 10. CONDITION_TAIL -> OR EXPRESSION CONDITION_TAIL
    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::OR] = {
        T(Terminal::OR), NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };
    // 11. CONDITION_TAIL -> eps (FOLLOW es ;)
    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::SEMICOLON] = { T(Terminal::EPSILON) };

    // 12. EXPRESSION -> IDENTIFIER REL_OP VALUE
    parse_table_[NonTerminal::EXPRESSION][Terminal::IDENTIFIER] = {
        T(Terminal::IDENTIFIER), NT(NonTerminal::REL_OP), NT(NonTerminal::VALUE)
    };

    // 13. REL_OP
    parse_table_[NonTerminal::REL_OP][Terminal::EQ] = { T(Terminal::EQ) };
    parse_table_[NonTerminal::REL_OP][Terminal::NEQ] = { T(Terminal::NEQ) };
    parse_table_[NonTerminal::REL_OP][Terminal::LT] = { T(Terminal::LT) };
    parse_table_[NonTerminal::REL_OP][Terminal::GT] = { T(Terminal::GT) };
    parse_table_[NonTerminal::REL_OP][Terminal::LE] = { T(Terminal::LE) };
    parse_table_[NonTerminal::REL_OP][Terminal::GE] = { T(Terminal::GE) };

    // 14. VALUE -> IDENTIFIER | NUMBER
    parse_table_[NonTerminal::VALUE][Terminal::IDENTIFIER] = { T(Terminal::IDENTIFIER) };
    parse_table_[NonTerminal::VALUE][Terminal::NUMBER] = { T(Terminal::NUMBER) };
}

std::string LL1Parser::getSymbolName(const Symbol& sym) {
    return sym.is_terminal ? term_names_[sym.term] : non_term_names_[sym.non_term];
}

Terminal LL1Parser::mapTokenToTerminal(const Token& token) {
    // Mapeo directo y exacto usando TokenType de minidb::lexer::TokenType
    switch (token.type) {
        case TokenType::kSelect: return Terminal::SELECT;
        case TokenType::kFrom: return Terminal::FROM;
        case TokenType::kWhere: return Terminal::WHERE;
        case TokenType::kAnd: return Terminal::AND;
        case TokenType::kOr: return Terminal::OR;
        case TokenType::kIdentifier: return Terminal::IDENTIFIER;
        case TokenType::kIntegerLiteral:
        case TokenType::kDecimalLiteral: return Terminal::NUMBER;
        case TokenType::kStar: return Terminal::STAR;
        case TokenType::kComma: return Terminal::COMMA;
        case TokenType::kSemicolon: return Terminal::SEMICOLON;
        case TokenType::kEqual: return Terminal::EQ;
        case TokenType::kNotEqual: return Terminal::NEQ;
        case TokenType::kLess: return Terminal::LT;
        case TokenType::kGreater: return Terminal::GT;
        case TokenType::kLessEqual: return Terminal::LE;
        case TokenType::kGreaterEqual: return Terminal::GE;
        case TokenType::kEndOfFile: return Terminal::END_OF_FILE;
        default: break;
    }

    // Fallback por si acaso se inicializó solo con lexeme
    std::string upper_lex = token.lexeme;
    std::transform(upper_lex.begin(), upper_lex.end(), upper_lex.begin(), ::toupper);

    if (upper_lex == "SELECT") return Terminal::SELECT;
    if (upper_lex == "FROM") return Terminal::FROM;
    if (upper_lex == "WHERE") return Terminal::WHERE;
    if (upper_lex == "AND") return Terminal::AND;
    if (upper_lex == "OR") return Terminal::OR;
    if (upper_lex == "*") return Terminal::STAR;
    if (upper_lex == ",") return Terminal::COMMA;
    if (upper_lex == ";") return Terminal::SEMICOLON;
    if (upper_lex == "=") return Terminal::EQ;
    if (upper_lex == "<>" || upper_lex == "!=") return Terminal::NEQ;
    if (upper_lex == "<") return Terminal::LT;
    if (upper_lex == ">") return Terminal::GT;
    if (upper_lex == "<=") return Terminal::LE;
    if (upper_lex == ">=") return Terminal::GE;

    if (!token.lexeme.empty() && std::isdigit(token.lexeme[0])) return Terminal::NUMBER;
    if (!token.lexeme.empty() && (std::isalpha(token.lexeme[0]) || token.lexeme[0] == '_')) return Terminal::IDENTIFIER;

    return Terminal::INVALID;
}

bool LL1Parser::parse(const std::vector<Token>& tokens) {
    std::stack<Symbol> stack;
    stack.push(Symbol::createTerminal(Terminal::END_OF_FILE));
    stack.push(Symbol::createNonTerminal(NonTerminal::STATEMENT));

    size_t token_idx = 0;

    std::cout << "\n" << std::left << std::setw(35) << "PILA SINTACTICA"
              << std::setw(25) << "ENTRADA RESTANTE"
              << "ACCION APLICADA" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    while (!stack.empty()) {
        Symbol top = stack.top();
        stack.pop();

        Terminal current_term = Terminal::END_OF_FILE;
        if (token_idx < tokens.size()) {
            current_term = mapTokenToTerminal(tokens[token_idx]);
        }

        std::stack<Symbol> temp_stack = stack;
        temp_stack.push(top);
        std::string stack_str = "";
        while(!temp_stack.empty()) {
            stack_str += getSymbolName(temp_stack.top()) + " ";
            temp_stack.pop();
        }
        if(stack_str.length() > 32) stack_str = stack_str.substr(0, 29) + "...";

        std::string input_str = "";
        for(size_t i = token_idx; i < std::min(tokens.size(), token_idx + 4); i++) {
            input_str += tokens[i].lexeme + " ";
        }
        if(token_idx >= tokens.size()) input_str = "$";
        if(input_str.length() > 22) input_str = input_str.substr(0, 19) + "...";

        if (top.is_terminal) {
            if (top.term == Terminal::EPSILON) {
                std::cout << std::left << std::setw(35) << stack_str << std::setw(25) << input_str
                          << "Match EPSILON (Pop)" << std::endl;
                continue;
            }

            if (top.term == current_term) {
                std::cout << std::left << std::setw(35) << stack_str << std::setw(25) << input_str
                          << "Match [" << getSymbolName(top) << "]" << std::endl;
                token_idx++;
            } else {
                std::size_t line = (token_idx < tokens.size()) ? tokens[token_idx].line : 1;
                std::size_t col = (token_idx < tokens.size()) ? tokens[token_idx].column : 1;
                std::cerr << "\n[ERROR SINTACTICO] En linea " << line << ", columna " << col << std::endl;
                std::cerr << "Token encontrado: " << term_names_[current_term] << std::endl;
                std::cerr << "Se esperaba: " << term_names_[top.term] << std::endl;
                return false;
            }
        } else {
            auto& row = parse_table_[top.non_term];
            if (row.find(current_term) != row.end()) {
                auto prod = row[current_term];
                std::string prod_str = getSymbolName(top) + " -> ";
                for (auto& s : prod) prod_str += getSymbolName(s) + " ";

                std::cout << std::left << std::setw(35) << stack_str << std::setw(25) << input_str
                          << prod_str << std::endl;

                for (auto it = prod.rbegin(); it != prod.rend(); ++it) {
                    stack.push(*it);
                }
            } else {
                std::size_t line = (token_idx < tokens.size()) ? tokens[token_idx].line : 1;
                std::size_t col = (token_idx < tokens.size()) ? tokens[token_idx].column : 1;
                std::cerr << "\n[ERROR SINTACTICO] En linea " << line << ", columna " << col
                          << ". No existe regla de derivacion para M["
                          << getSymbolName(top) << ", " << term_names_[current_term] << "]" << std::endl;
                std::cerr << "Se esperaban: ";
                for (auto const& [key, val] : row) {
                    std::cerr << term_names_[key] << " ";
                }
                std::cerr << "\nRecibido: " << term_names_[current_term] << std::endl;
                return false;
            }
        }
    }
    std::cout << "\n[EXITO] Parseo LL(1) completado. La sentencia SQL es valida." << std::endl;
    return true;
}

} // namespace minidb