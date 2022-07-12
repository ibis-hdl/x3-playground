//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

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

using char_parser::dec_integer;
using x3::char_;

// clang-format off
auto const exponent = [](auto&& signs) {
    using CharT = decltype(signs);
    return x3::rule<struct exponent_class, std::string>{ "exponent" } = x3::as_parser(
        x3::omit[ char_("Ee") ]
        >> x3::raw[ x3::lexeme [
             -char_(std::forward<CharT>(signs)) >> dec_integer
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

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct decimal_real_parser : x3::parser<decimal_real_parser> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_integer;
        using detail::signed_exp;

        auto const grammar =  // use lexeme[] from outer parser
            dec_integer >> '.' >> x3::expect[dec_integer] >> -signed_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);
        attribute.base = 10U;  // decimal literal has base = 10

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec;
        attribute.value = convert::real<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct decimal_integer_parser : x3::parser<decimal_integer_parser> {
    using attribute_type = ast::integer_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_integer;
        using detail::unsigned_exp;
        using x3::lit;

        auto const grammar =  // use lexeme[] from outer parser; exclude based literal
            dec_integer >> !lit('#') >> -unsigned_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);
        attribute.base = 10U;  // decimal literal has base = 10

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec;
        attribute.value = convert::integer<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static decimal_integer_parser const decimal_integer = {};
static decimal_real_parser const decimal_real = {};

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct decimal_literal_parser : x3::parser<decimal_literal_parser> {
    using attribute_type = ast::decimal_literal;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);

        auto const begin = first;

        auto const grammar = x3::lexeme[
            (decimal_real | decimal_integer)
        ];

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
