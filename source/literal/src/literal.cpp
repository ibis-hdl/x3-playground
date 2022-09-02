//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

//#define BOOST_SPIRIT_X3_DEBUG
#include <literal/ast.hpp>
#include <literal/parser/decimal_literal.hpp>
#include <literal/parser/bit_string_literal.hpp>
#include <literal/parser/based_literal.hpp>
#include <literal/parser/identifier.hpp>
#include <literal/parser/error_handler.hpp>

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

// [How do I get which() to work correctly in boost spirit x3 expectation_failure?](
// https://stackoverflow.com/questions/71281614/how-do-i-get-which-to-work-correctly-in-boost-spirit-x3-expectation-failure)
template <typename RuleID, typename AttributeT = x3::unused_type>
auto as(auto p, char const* name = typeid(decltype(p)).name())
{
    using tag = my_x3_error_handler<RuleID>;
    return x3::rule<tag, AttributeT>{ name } = p;
}

template <typename RuleID, typename AttributeT>
auto mandatory(auto p, char const* name = typeid(decltype(p)).name())
{
    return x3::expect[as<RuleID, AttributeT>(p, name)];
}

// [Getting custom error on rule level using Spirit X3](
//  https://stackoverflow.com/questions/72841446/getting-custom-error-on-rule-level-using-spirit-x3)
template <typename RuleID, typename AttributeT>
struct mandatory_type {
    template <typename Expr>
    auto with_error_handler(Expr&& expr, char const* name = typeid(decltype(expr)).name()) const
    {
        using tag = my_x3_error_handler<RuleID>;
        return x3::rule<tag, AttributeT>{ name } = x3::as_parser(std::forward<Expr>(expr));
    }
    template <typename Expr>
    auto operator()(Expr&& expr, char const* name = typeid(decltype(expr)).name()) const
    {
        return x3::expect[with_error_handler<RuleID, AttributeT>(expr, name)];
    }
};
template <typename RuleID, typename T>
static const mandatory_type<RuleID, T> mandatory_ = {};

struct based_literal_class;
struct bit_string_literal_class;
struct grammar_class;

using x3::char_;
using x3::lit;

// clang-format off

static auto const c_style_comments = "/*" >> x3::lexeme[*(x3::char_ - "*/")] >> "*/";
static auto const cpp_style_comment = "//" >> *~x3::char_("\r\n");
static auto const comment = cpp_style_comment | c_style_comments;

// BNF: based_literal := base # based_integer [ . based_integer ] # [ exponent ]
auto const based_literal = x3::rule<struct based_literal_class, ast::based_literal>{ "based literal" } =
    parser::based_literal;

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
auto const decimal_literal = x3::rule<struct decimal_literal_class, ast::decimal_literal>{ "decimal literal" } =
    parser::decimal_literal;

// BNF: abstract_literal ::= decimal_literal | based_literal
// Note: {decimal, based}_literal's AST nodes does have same memory layout now!
// This would allow to treat them as abstract_literal without variant type; only
// parsers would be different!
auto const abstract_literal = x3::rule<struct abstract_literal_class, ast::abstract_literal>{ "based or decimal abstract literal" } =
    based_literal | decimal_literal
    ;

// Note, the LRM doesn't specify the allowed characters, hence it's assumed
// that it follows the natural conventions.
auto const unit_name = x3::rule<struct unit_name_class, std::string>{ "unit name" } =
    x3::lexeme[ +x3::alpha ]
    ;

// BNF: physical_literal ::= [ abstract_literal ] unit_name
auto const physical_literal = x3::rule<struct physical_literal_class, ast::physical_literal>{ "physical literal" } =
    -abstract_literal >> unit_name;
    ;

// BNF: numeric_literal ::= abstract_literal | physical_literal
auto const numeric_literal = x3::rule<struct numeric_literal_class, ast::numeric_literal>{ "numeric literal" } =
    physical_literal | abstract_literal // order matters
    ;

// BNF: character_literal ::= ' graphic_character '
auto const character_literal = x3::rule<struct character_literal_class, ast::character_literal>{ "character literal" } =
    x3::lexeme ["\'" >> ( ( x3::graph - "\'" ) | "\'" ) >> "\'" ]
    ;

// BNF: enumeration_literal ::= identifier | character_literal
auto const enumeration_literal = x3::rule<struct enumeration_literal_class, ast::enumeration_literal>{ "enumeration literal" } =
    parser::identifier | character_literal
    ;

auto const grammar = x3::rule<struct grammar_class, ast::literals>{ "grammar" } =
    x3::skip(x3::space | comment)[
          *(lit("X :=")
        >> mandatory<grammar_class, ast::literal>(
            (abstract_literal | parser::bit_string_literal)
           , "correct grammar")
        >> x3::expect[';'])
    ];

struct based_literal_class : my_x3_error_handler<based_literal_class> {};
struct bit_string_literal_class : my_x3_error_handler<bit_string_literal_class> {};
struct character_literal_class : my_x3_error_handler<character_literal_class> {};
struct grammar_class : my_x3_error_handler<grammar_class> {};

// clang-format on

}  // namespace

int main()
{
    using namespace ast;

    std::string const input = R"(
    // bit string literal
    X := b"1000_0001";
    X := x"AFFE_Cafe";
    X := O"777";
    //X := X""; // FixMe: empty also allowed
    // decimal literal
    X := 42;
    X := 1e+3;
    X := 42.42;
    X := 2.2E-6;
    X := 3.14e+1;
    // based literal
    X := 8#1_20#E1;
    X := 0_2#1100_0001#;
    X := 10#42#E4;
    X := 16#AFFE_1.0Cafe#;
    X := 16#AFFE_2.0Cafe#e-10;
    X := 16#DEAD_BEEF#e+0;
    // from LRM93
    X := 2#1111_1111#;  // 255
    X := 016#00FF#;     // 255
    X := 16#E#E1;       // 224
    X := 2#1110_0000#;  // 224
    X := 16#F.FF#E+2;   // 4095.0
    X := 2#1.1111_1111_111#E11; // 4095.0
    // failure test: bit string literal
    X := x"AFFE_Cafee"; // 'from_chars': Numerical result out of range
/*
    // failure test
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
        using error_handler_type = x3::error_handler<decltype(input.begin())>;
        error_handler_type error_handler(input.begin(), input.end(), std::cerr, "input");

        auto const grammar_ = x3::with<x3::error_handler_tag>(error_handler)[grammar >> x3::eoi];

        ast::literals literals;
        auto first = input.begin();
        bool parse_ok = x3::parse(first, input.end(), grammar_, literals);

        std::cout << fmt::format("complete parse ok is '{}'\nnumeric literals:\n", parse_ok);
        for (auto const& lit : literals) {
            std::cout << "result: " << lit << '\n';
        }
        if (!parse_ok) {
            // std::cout << "Rest:\n----8<----\n" << std::string(first, input.end()) <<
            // "---->8----\n";
        }
    }
    catch (std::exception const& e) {
        std::cerr << fmt::format("caught '{}'\n", e.what());
    }
    catch (...) {
        std::cerr << "caught 'Unexpected exception'\n";
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
