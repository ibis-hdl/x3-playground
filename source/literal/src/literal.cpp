//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/decimal_literal.hpp>
#include <literal/parser/bit_string_literal.hpp>
#include <literal/parser/based_literal.hpp>
#include <literal/parser/identifier.hpp>
#include <literal/parser/comment.hpp>
#include <literal/parser/error_handler.hpp>
#include <literal/parser/parser_id.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <fmt/format.h>
#include <range/v3/view/filter.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace {

// TODO: Check on useability of
// [How do I get which() to work correctly in boost spirit x3 expectation_failure?](
// https://stackoverflow.com/questions/71281614/how-do-i-get-which-to-work-correctly-in-boost-spirit-x3-expectation-failure)
// The compile problem rises with my_x3_error_handler::id() member function which
// requires full defined types to print just this one! The my_x3_error_handler
// here is templated.

// clang-format off

// BNF: based_literal := base # based_integer [ . based_integer ] # [ exponent ]
auto const based_literal = x3::rule<based_literal_class, ast::based_literal>{ "based literal" } =
    parser::based_literal;

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
auto const decimal_literal = x3::rule<decimal_literal_class, ast::decimal_literal>{ "decimal literal" } =
    parser::decimal_literal;

// BNF: abstract_literal ::= decimal_literal | based_literal
// Note: {decimal, based}_literal's AST nodes does have same memory layout!
auto const abstract_literal = x3::rule<struct abstract_literal_class, ast::abstract_literal>{ "based or decimal abstract literal" } =
    based_literal | decimal_literal
    ;

// Note, the LRM doesn't specify the allowed characters, hence it's assumed
// that it follows the natural conventions.
auto const unit_name = x3::rule<struct unit_name_class, std::string>{ "unit name" } =
    x3::lexeme[ +x3::alpha ]
    ;

// BNF: physical_literal ::= [ abstract_literal ] unit_name
// ATTENTION: In the BNF, the abstract_literal is optional. This results in a stand-alone
// unit_name, with the result of the default constructed based_literal (with base = 0) with
// any unit_name, such as NULL (the keyword).
// This must be taken into account when implementing the secondary_unit_declaration!
auto const physical_literal = x3::rule<struct physical_literal_class, ast::physical_literal>{ "physical literal" } =
    abstract_literal >> unit_name;
    ;

// BNF: numeric_literal ::= abstract_literal | physical_literal
auto const numeric_literal = x3::rule<struct numeric_literal_class, ast::numeric_literal>{ "numeric literal" } =
    physical_literal | abstract_literal // order matters
    ;

auto const graphic_character = x3::graph | x3::space;

// BNF: character_literal ::= ' graphic_character '
auto const character_literal = x3::rule<character_literal_class, ast::character_literal>{ "character literal" } =
    x3::lexeme ["\'" >> x3::expect[( graphic_character - "\'" ) | x3::char_("\'")] >> "\'" ]
    ;

// BNF: enumeration_literal ::= identifier | character_literal
auto const enumeration_literal = x3::rule<struct enumeration_literal_class, ast::enumeration_literal>{ "enumeration literal" } =
    parser::identifier | character_literal
    ;

// BNF: string_literal ::= " { graphic_character } "
namespace detail {
auto const string_literal_charset = [](char delim) {
    return x3::rule<struct _, std::string>{ "string literal charset" } =
    *(   ( graphic_character - x3::char_(delim)  )
       | ( x3::char_(delim) >> x3::char_(delim) )
    )
    ;
};
auto const string_literal = [](char delim) {
    return x3::rule<struct _, std::string>{ "string literal" } =
        x3::lit(delim) >> x3::raw[string_literal_charset(delim)] >> x3::lit(delim)
    ;
};
} // namespace detail

auto const string_literal = x3::rule<string_literal_class, ast::string_literal>{ "string literal" } =
    x3::lexeme [
           detail::string_literal('"') | detail::string_literal('%')
    ];

// BNF: literal ::= numeric_literal | enumeration_literal | string_literal | bit_string_literal | null
auto const literal = x3::rule<struct literal_class, ast::literal>{ "literal" } = // order matters
    parser::NULL_ | enumeration_literal | string_literal | parser::bit_string_literal | numeric_literal
    ;

auto const literal_rule = x3::rule<literal_rule_class, ast::literal>{ "literal" } =
    x3::eps > "X" > ":=" > literal > ';'
    ;

auto const grammar = x3::rule<grammar_class, ast::literals>{ "grammar" } =
    x3::skip(x3::space | parser::comment)[
        *literal_rule >> x3::eoi
    ];

template<typename RuleID>
using my_x3_error_handler = parser::my_x3_error_handler<RuleID>;


// clang-format on

}  // namespace

int main()
{
    std::ios_base::sync_with_stdio(false);

    using namespace ast;

    std::string const input = R"(
    // Keyword NULL
    X := null;
    // bit string literal
    X := b"1000_0001";
    X := x"AFFE_Cafe";
    X := O"777";
    X := X"";           // empty bit string literal is allowed

    // decimal literal
    X := 42;
    X := 1e+3;
    X := 42.42;
    X := 2.2E-6;
    X := 3.14e+1;

    // based literal
    X := 4#1_20#E1;     // 96 - yes, uncommon base for integers are (weak) supported
    X := 8#1_20#E1;
    X := 0_2#1100_0001#;
    X := 10#42#E4;
    X := 16#AFFE_1.0Cafe#;
    X := 16#AFFE_2.0Cafe#e-10;
    X := 16#DEAD_BEEF#e+0;

    // string literal
    X := "setup time too small";
    X := " ";
    X := "a";
    X := """";
    X := "";            // empty string literal is allowed

    // char literal
    X := '0';
    X := 'A';
    X := '*';
    X := ''';
    X := ' ';
    X := '';            // empty char literal is *not* allowed

    // numeric/physical literal
    X := 10.7 ns;       // decimal (real)
    X := 42 us;         // decimal (real)
    X := 10#42#E4 kg;   // based literal

    // mixed types from LRM93
    X := 2#1111_1111#;  // 255
    X := 016#00FF#;     // 255
    X := 16#E#E1;       // 224
    X := 2#1110_0000#;  // 224
    X := 16#F.FF#E+2;   // 4095.0
    X := 2#1.1111_1111_111#E11; // 4095.0
    // enumeration_literal
    X := id;
    X := 'e';

    // failure test: bit string literal
    X := x"AFFE_Cafee"; // 'from_chars': Numerical result out of range

    // failure test: base out of range
    X := 666#9#; // FIXME error location indicator
/*
    // other failure tests
    X := 2##;          // -> based literal real or integer type
    X := 3#011#;       // base not supported
    X := 2#120#1;      // wrong char set for binary
    X := 10#42#666;    // exp can't fit double (e308)
    X := 8#1#e1        // forgot ';' - otherwise ok
    X := 8#2#          // also forgot ';' - otherwise ok
    X := 16#1.2#e;     // forgot exp num
*/
    // ok, just to test error recovery afterwards
    X := 10#42.666#e-4;
)";

    try {
        auto iter = input.begin();
        auto const end = input.end();
        using error_handler_type = x3::error_handler<decltype(input.begin())>;
        error_handler_type error_handler(input.begin(), end, std::cerr, "input");

        auto const grammar_ = x3::with<x3::error_handler_tag>(error_handler)[grammar];

        ast::literals literals;
        bool parse_ok = x3::parse(iter, end, grammar_, literals);

        std::cout << fmt::format("parse success: {}\n", parse_ok);
        if(!literals.empty()) {
            std::cout << "numeric literals:\n";
            for (auto const& lit : literals) {
                std::cout << " - " << lit << '\n';
            }
        }
        if(iter != end) {
            std::cout << "Remaining: " << std::quoted(std::string_view{iter, end}) << "\n";
        }
    }
    catch (std::exception const& e) {
        std::cerr << fmt::format("caught in main() '{}'\n", e.what());
    }
    catch (...) {
        std::cerr << "caught in main() 'Unexpected exception'\n";
    }
}

// https://stackoverflow.com/search?q=user%3A85371+x3+parser_base
// https://stackoverflow.com/questions/49722452/combining-rules-at-runtime-and-returning-rules/49722855#49722855

/*
 stateful context x3::width[]

[Boost spirit x3 - lazy
parser](https://stackoverflow.com/questions/60171119/boost-spirit-x3-lazy-parser/60176802#60176802)
[Boost Spirit X3 cannot compile repeat directive with variable
factor](https://stackoverflow.com/questions/33624149/boost-spirit-x3-cannot-compile-repeat-directive-with-variable-factor/33627991#33627991)
=> https://coliru.stacked-crooked.com/a/f778d7b2e11cfcb5
*/
