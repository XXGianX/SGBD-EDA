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

    parse_table_[NonTerminal::STATEMENT][Terminal::SELECT] = {
        T(Terminal::SELECT), NT(NonTerminal::SELECT_LIST), T(Terminal::FROM),
        T(Terminal::IDENTIFIER), NT(NonTerminal::WHERE_CLAUSE), T(Terminal::SEMICOLON)
    };

    parse_table_[NonTerminal::SELECT_LIST][Terminal::STAR] = { T(Terminal::STAR) };
    parse_table_[NonTerminal::SELECT_LIST][Terminal::IDENTIFIER] = {
        T(Terminal::IDENTIFIER), NT(NonTerminal::COLUMN_LIST_TAIL)
    };

    parse_table_[NonTerminal::COLUMN_LIST_TAIL][Terminal::COMMA] = {
        T(Terminal::COMMA), T(Terminal::IDENTIFIER), NT(NonTerminal::COLUMN_LIST_TAIL)
    };
    parse_table_[NonTerminal::COLUMN_LIST_TAIL][Terminal::FROM] = { T(Terminal::EPSILON) };

    parse_table_[NonTerminal::WHERE_CLAUSE][Terminal::WHERE] = {
        T(Terminal::WHERE), NT(NonTerminal::CONDITION)
    };
    parse_table_[NonTerminal::WHERE_CLAUSE][Terminal::SEMICOLON] = { T(Terminal::EPSILON) };

    parse_table_[NonTerminal::CONDITION][Terminal::IDENTIFIER] = {
        NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };

    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::AND] = {
        T(Terminal::AND), NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };
    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::OR] = {
        T(Terminal::OR), NT(NonTerminal::EXPRESSION), NT(NonTerminal::CONDITION_TAIL)
    };
    parse_table_[NonTerminal::CONDITION_TAIL][Terminal::SEMICOLON] = { T(Terminal::EPSILON) };

    parse_table_[NonTerminal::EXPRESSION][Terminal::IDENTIFIER] = {
        T(Terminal::IDENTIFIER), NT(NonTerminal::REL_OP), NT(NonTerminal::VALUE)
    };

    parse_table_[NonTerminal::REL_OP][Terminal::EQ] = { T(Terminal::EQ) };
    parse_table_[NonTerminal::REL_OP][Terminal::NEQ] = { T(Terminal::NEQ) };
    parse_table_[NonTerminal::REL_OP][Terminal::LT] = { T(Terminal::LT) };
    parse_table_[NonTerminal::REL_OP][Terminal::GT] = { T(Terminal::GT) };
    parse_table_[NonTerminal::REL_OP][Terminal::LE] = { T(Terminal::LE) };
    parse_table_[NonTerminal::REL_OP][Terminal::GE] = { T(Terminal::GE) };

    parse_table_[NonTerminal::VALUE][Terminal::IDENTIFIER] = { T(Terminal::IDENTIFIER) };
    parse_table_[NonTerminal::VALUE][Terminal::NUMBER] = { T(Terminal::NUMBER) };
}

std::string LL1Parser::getSymbolName(const Symbol& sym) {
    return sym.is_terminal ? term_names_[sym.term] : non_term_names_[sym.non_term];
}

Terminal LL1Parser::mapTokenToTerminal(const Token& token) {
    std::string lex = token.lexeme; // <- AJUSTAR AQUI SI SU TOKEN TIENE OTRO ATRIBUTO O GETTER
    std::string upper_lex = lex;
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

    if (!lex.empty() && std::isdigit(lex[0])) return Terminal::NUMBER;
    if (!lex.empty() && (std::isalpha(lex[0]) || lex[0] == '_')) return Terminal::IDENTIFIER;

    return Terminal::INVALID;
}

bool LL1Parser::parse(const std::vector<Token>& tokens) {
    std::stack<Symbol> stack;
    stack.push(Symbol::createTerminal(Terminal::END_OF_FILE));
    stack.push(Symbol::createNonTerminal(NonTerminal::STATEMENT));

    size_t token_idx = 0;

    std::cout << "\n" << std::left << std::setw(40) << "PILA SINTACTICA"
              << std::setw(30) << "ENTRADA RESTANTE"
              << "ACCION APLICADA" << std::endl;
    std::cout << std::string(100, '-') << std::endl;

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
        if(stack_str.length() > 37) stack_str = stack_str.substr(0, 34) + "...";

        std::string input_str = "";
        for(size_t i = token_idx; i < std::min(tokens.size(), token_idx + 4); i++) {
            input_str += tokens[i].lexeme + " "; // <- AJUSTAR AQUI TAMBIEN SI EL LEXEMA ES PRIVADO
        }
        if(token_idx >= tokens.size()) input_str = "$";
        if(input_str.length() > 27) input_str = input_str.substr(0, 24) + "...";

        if (top.is_terminal) {
            if (top.term == Terminal::EPSILON) {
                std::cout << std::left << std::setw(40) << stack_str << std::setw(30) << input_str
                          << "Match EPSILON (Pop)" << std::endl;
                continue;
            }

            if (top.term == current_term) {
                std::cout << std::left << std::setw(40) << stack_str << std::setw(30) << input_str
                          << "Match [" << getSymbolName(top) << "]" << std::endl;
                token_idx++;
            } else {
                std::cerr << "\n[ERROR SINTACTICO] En el token indice " << token_idx << std::endl;
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

                std::cout << std::left << std::setw(40) << stack_str << std::setw(30) << input_str
                          << prod_str << std::endl;

                for (auto it = prod.rbegin(); it != prod.rend(); ++it) {
                    stack.push(*it);
                }
            } else {
                std::cerr << "\n[ERROR SINTACTICO] No existe regla de derivacion para M["
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
