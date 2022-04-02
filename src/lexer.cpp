#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

bool operator==(const Token &lhs, const Token &rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token &lhs, const Token &rhs) {
    return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const Token &rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type)                                                                   \
    if (auto p = rhs.TryAs<type>())                                                           \
        return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type)                                                                 \
    if (rhs.Is<type>())                                                                       \
        return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

std::unordered_map<std::string, Token> keywords = {
    {"class", token_type::Class()},    {"return", token_type::Return()},
    {"if", token_type::If()},          {"else", token_type::Else()},
    {"def", token_type::Def()},        {"print", token_type::Print()},
    {"or", token_type::Or()},          {"None", token_type::None()},
    {"and", token_type::And()},        {"True", token_type::True()},
    {"False", token_type::False()},    {"not", token_type::Not()},
    {"==", token_type::Eq()},          {"!=", token_type::NotEq()},
    {">=", token_type::GreaterOrEq()}, {"<=", token_type::LessOrEq()}};

Lexer::Lexer(std::istream &input) : input_(input) {
    if (input_) {
        current_token_ = token_type::Newline{};
    }
    indent_level_ = 0;
    current_token_ = GetNextToken();
}

Token Lexer::NextToken() {
    if (input_) {
        current_token_ = GetNextToken();
    } else {
        current_token_ = token_type::Eof{};
    }

    return current_token_;
}

void Lexer::IgnoreEmptyLines() {
    while (input_.peek() == '\n') {
        input_.get();
    }
}

Token Lexer::GetNextToken() {

    if (current_token_ == token_type::Newline{} || indent_level_) {
        IgnoreEmptyLines();
        Token indent_token = GetIndent();
        if (indent_token != token_type::None{}) {
            return indent_token;
        }
    }

    while (input_) {
        char ch = input_.peek();

        if (ch == ' ') {
            input_.get();
            continue;
        }

        if (ch == '\n') {
            input_.get();
            return token_type::Newline{};
        }

        if (ch == '#') {
            input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            IgnoreEmptyLines();
            if (input_.peek() != EOF && current_token_ != token_type::Newline{}) {
                return token_type::Newline{};
            }
            continue;
        }

        if (std::ispunct(ch)) {
            if (ch == '\"' || ch == '\'') {
                return GetString();
            } else if (ch == '_') {
                return GetId();
            } else if (ch == '!' || ch == '=' || ch == '>' || ch == '<') {
                return GetCompOperator();
            } else {
                char c = input_.get();
                return token_type::Char{c};
            }
        }

        if (std::isalpha(ch)) {
            return GetId();
        }

        if (std::isdigit(ch)) {
            return GetDigit();
        }
    }

    if (current_token_ != token_type::Newline{}) {
        input_.clear();
        input_.unget();
        char c = input_.get();
        if (isalnum(c) || ispunct(c)) {
            return token_type::Newline{};
        }
    }

    return token_type::Eof{};
}

int Lexer::GetIndentLevel() {
    static size_t prev_indent{};

    size_t space_count{};
    while (input_.peek() == ' ') {
        input_.get();
        space_count++;
    }
    int now_indent = (space_count / 2u);
    int level = now_indent - prev_indent;
    prev_indent = now_indent;

    return level;
}

Token Lexer::GetIndent() {
    if (indent_level_ == 0) {
        indent_level_ = GetIndentLevel();
    }
    if (indent_level_ > 0) {
        indent_level_--;
        return token_type::Indent{};
    } else if (indent_level_ < 0) {
        indent_level_++;
        return token_type::Dedent{};
    }
    return token_type::None{};
}

Token Lexer::GetId() {
    std::string id_word{};
    while (input_.peek() != EOF) {
        char ch = input_.peek();
        if (iscntrl(ch) || std::isspace(ch) || (std::ispunct(ch) && ch != '_')) {
            break;
        }
        id_word += input_.get();
    }
    if (keywords.count(id_word)) {
        return keywords.at(id_word);
    }
    return token_type::Id{id_word};
}

Token Lexer::GetCompOperator() {
    char c = input_.get();
    if (input_.peek() == '=') {
        std::string op{c};
        op += input_.get();
        return keywords.at(op);
    }
    return token_type::Char{c};
}

Token Lexer::GetString() {
    string str;
    char end_quote = input_.get();
    while (input_.peek() != end_quote) {
        char ch = input_.get();
        if (ch == '\\') {
            char s = input_.get();
            switch (s) {
            case 'n':
                str.push_back('\n');
                break;

            case 't':
                str.push_back('\t');
                break;

            case '\"':
                str.push_back('\"');
                break;

            case '\'':
                str.push_back('\'');
                break;

            default:
                str.push_back(ch);
                input_.unget();
                break;
            }
            continue;
        }
        str.push_back(ch);
    }
    input_.get();
    return token_type::String{str};
}

Token Lexer::GetDigit() {
    int n;
    input_ >> n;
    return token_type::Number{n};
}

} // namespace parse