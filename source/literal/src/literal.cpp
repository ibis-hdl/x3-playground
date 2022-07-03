//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

//#define BOOST_SPIRIT_X3_DEBUG
#include <literal/ast.hpp>
#include <literal/parser/integer_parser.hpp>
#include <literal/parser/literal_base_parser.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/optional.hpp>
#include <fmt/format.h>
#include <range/v3/view/filter.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace {

// [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
// https://coliru.stacked-crooked.com/a/7fd3fbc8b2452590
// for different error recovery strategies
template <typename RuleID>
struct my_error_handler {  // try to recover and continue to parse
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& e, Ctx const& ctx) const
    {
        std::cerr << "error handler for RuleID: '" << typeid(RuleID).name() << "'\n";
        auto& error_handler = x3::get<x3::error_handler_tag>(ctx);
        std::string message = "Error! Expecting " + e.which() + " here:";
        error_handler(e.where(), message);
        // FixMe: always true: e.where() == first ; see
        // https://github.com/boostorg/spirit/issues/726 our error resolution strategy
        auto const position = std::string_view(first, last).find_first_of(";");
        if (position != std::string_view::npos) {
            std::advance(first, position + 1);  // move iter behind
            std::cout << fmt::format("Rest:\n---8<---\n{}\n--->8---\n",
                                     std::string_view(first, last));
        }
        else {
            first = last;  // no way here :(
            return x3::error_handler_result::fail;
        }
        return x3::error_handler_result::accept;
    }
};

// [How do I get which() to work correctly in boost spirit x3 expectation_failure?](
// https://stackoverflow.com/questions/71281614/how-do-i-get-which-to-work-correctly-in-boost-spirit-x3-expectation-failure)
template <typename RuleID, typename AttributeT = x3::unused_type>
auto as(auto p, char const* name = typeid(decltype(p)).name())
{
    using tag = my_error_handler<RuleID>;
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
        using tag = my_error_handler<RuleID>;
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

auto const comment = "//" >> *(char_ - x3::eol) >> x3::eol;

auto const delimit_numeric_digits = [](auto&& char_range, char const* name) {
    auto const cs = x3::char_(char_range);
    return as<std::string>(x3::raw[ cs >> *('_' >> +cs | cs) ], name);
};
auto const bin_digits = delimit_numeric_digits("01", "binary digits");
auto const oct_digits = delimit_numeric_digits("0-7", "octal digits");
auto const dec_digits = delimit_numeric_digits("0-9", "decimal digits");
auto const hex_digits = delimit_numeric_digits("0-9a-fA-F", "hexadecimal digits");


auto const base_ = parser::literal_base_parser{};
auto const int_ =  parser::integer_parser<10U, std::int32_t>{};
auto const uint_ = parser::integer_parser<10U, std::uint32_t>{};

auto const base = x3::rule<struct _, std::uint32_t>{ "literal base" } =
    base_;
auto const signed_exp = x3::rule<struct _, std::int32_t>{ "literal real exponent" } =
    x3::omit[ char_("Ee") ] >> int_;
auto const unsigned_exp = x3::rule<struct _, std::uint32_t>{ "literal integer exponent" } =
    x3::omit[ char_("Ee") >> -lit('+') ] >> uint_;
auto const based_digit = x3::rule<struct _, std::string>{ "literal digit" } =
    x3::raw[ x3::xdigit >> *( -lit('_') >> x3::xdigit) ]; // FixMe: char set depends on base
auto const real = x3::rule<struct _, ast::real_type>{ "literal real" } =
    based_digit >> '.' >> x3::expect[ based_digit ] >> '#' >> -signed_exp;
auto const integer = x3::rule<struct _, ast::integer_type>{ "literal integer" } =
    based_digit >> '#' >> -unsigned_exp;

// BNF: based_literal := base # based_integer [ . based_integer ] # [ exponent ]
auto const based_literal = x3::rule<struct based_literal_class, ast::based_literal>{ "based literal" } =
    x3::lexeme[
           base >> '#' >> mandatory<based_literal_class, ast::based_literal::num_type>( real | integer, "based literal real or integer type")
    ];

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
auto const decimal_literal = x3::rule<struct decimal_literal_class, ast::decimal_literal>{ "decimal literal" } =
    (real | integer) >> x3::attr(10U);

// BNF: decimal_literal | based_literal
// Note: {decimal, based}_literal's AST nodes does have same memory layout now!
// This would allow to treat them as abstract_literal without variant type; only
// parsers would be different!
auto const abstract_literal = x3::rule<struct abstract_literal_class, ast::abstract_literal>{ "based or decimal abstract literal" } =
    based_literal | decimal_literal;

// BNF: base_specifier " [ bit_value ] "
// [Boost spirit x3 - lazy parser with compile time known parsers, referring to a previously matched value](
//  https://stackoverflow.com/questions/72833517/boost-spirit-x3-lazy-parser-with-compile-time-known-parsers-referring-to-a-pr)
auto const bit_string_literal = x3::rule<struct bit_string_literal_class, ast::bit_string_literal>{ "bit string literal" } =
      (x3::omit[ char_("Bb") >> x3::expect['"'] ] >> x3::attr(2)
        >> mandatory<bit_string_literal_class, std::string>(lit('"') >> -bin_digits >> lit('"'), "sequence of binary digits"))
    | (x3::omit[ char_("Xx") >> x3::expect['"'] ] >> x3::attr(16)
        >> mandatory<bit_string_literal_class, std::string>(lit('"') >> -hex_digits >> lit('"'), "sequence of hexadecimal digits"))
    | (x3::omit[ char_("Oo") >> x3::expect['"'] ] >> x3::attr(8)
        >> mandatory<bit_string_literal_class, std::string>(lit('"') >> -oct_digits >> lit('"'), "sequence of octal digits"))
    ;

auto const grammar = x3::rule<struct grammar_class, ast::literals>{ "grammar" } =
    x3::skip(x3::space | comment)[
          *(lit("X :=")
        >> mandatory<grammar_class, ast::literal>((abstract_literal | bit_string_literal), "literal")
        >> x3::expect[';'])
    ];

struct based_literal_class : my_error_handler<based_literal_class> {};
struct bit_string_literal_class : my_error_handler<bit_string_literal_class> {};
struct grammar_class : my_error_handler<grammar_class> {};

// clang-format on

}  // namespace

int main()
{
    using namespace ast;

    std::string const input = R"(
    // based literal
    X := 0_2#1100_0001#;
    X := 8#1_20#E1;
    X := 10#42#E42;
    X := 16#AFFE_1.0CAFE#e-10;
    // decimal literal
    X := 42;
    X := 42.42;
    X := 2.2E-6;
    X := 1e+3;
    // bit string literal
    X := b"1000_0001";
    X := x"AFFE_CAFFE";
    // failure test
    X := 2##;          // -> based literal real or integer type
    X := 3#011#;       // base not supported
    X := 2#120#1;      // wrong char set for binary
    X := 10#42#666;    // exp can't fit double (e308)
    X := 8#1#e1        // forgot ';' - otherwise ok
    X := 8#2#          // also forgot ';' - otherwise ok
    X := 16#1.2#e;     // forgot exp num
    // ok, just to test error recovery afterwards
    X := 10#42.666#e4711;
)";

    try {
        using error_handler_type = x3::error_handler<decltype(input.begin())>;
        error_handler_type error_handler(input.begin(), input.end(), std::cerr);

        auto const grammar_ = x3::with<x3::error_handler_tag>(error_handler)[grammar >> x3::eoi];

        ast::literals literals;
        bool parse_ok = x3::parse(input.begin(), input.end(), grammar_, literals);
        std::cout << fmt::format("parse ok is '{}', numeric literals:\n", parse_ok);
        for (auto const& lit : literals) {
            std::cout << lit << '\n';
        }
    }
    catch (std::exception const& e) {
        std::cerr << fmt::format("caught {}\n", e.what());
    }
    catch (...) {
        std::cerr << "caught unexpected exception\n";
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