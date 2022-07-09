//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/convert.hpp>

#include <boost/spirit/home/x3.hpp>

#include <cmath>
#include <limits>

#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>

#include <iostream>
#include <iomanip>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace detail {

using char_parser::dec_digits;
using x3::char_;

// clang-format off
auto const exponent = [](auto&& signs) {
    using CharT = decltype(signs);
    return x3::rule<struct exponent_class, std::string>{ "exponent" } = x3::as_parser(
        x3::omit[ char_("Ee") ]
        >> x3::raw[ x3::lexeme [
             -char_(std::forward<CharT>(signs)) >> dec_digits
        ]]
    );
};
// clang-format on

// FixMe: Limit digits

// clang-format off
auto const signed_exp = x3::rule<struct _, std::string>{ "real exponent" } =
    exponent("-+");

auto const unsigned_exp = x3::rule<struct _, std::string>{ "integer exponent" } =
    exponent('+');
// clang-format on

}  // namespace detail

namespace util {

// [cppreference.com std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
// helper type for the visitor
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename Expr>
std::string inspect_expr(Expr const&)
{
    return boost::core::demangle(typeid(Expr).name());
}

}  // namespace util

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct real_parser_ng : x3::parser<real_parser_ng> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_digits;
        using detail::signed_exp;

        auto const grammar =  //
            dec_digits >> '.' >> x3::expect[dec_digits] >> -signed_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec{};
        attribute.value = convert::real10<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct integer_parser_ng : x3::parser<integer_parser_ng> {
    using attribute_type = ast::integer_type;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_digits;
        using detail::unsigned_exp;
        using x3::lit;

        auto const grammar =  // exclude based literal
            dec_digits >> !lit('#') >> -unsigned_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec{};
        attribute.value = convert::integer10<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static integer_parser_ng const integer_ng = {};
static real_parser_ng const real_ng = {};

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct decimal_literal_parser : x3::parser<decimal_literal_parser> {
    using attribute_type = ast::decimal_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        auto const grammar = (real_ng | integer_ng) >> x3::attr(10U);

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return true;
    }

};

static decimal_literal_parser const decimal_literal = {};

}  // namespace parser
