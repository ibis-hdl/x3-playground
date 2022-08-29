//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/parser/error_handler.hpp>
//#include <literal/parser/util/iterator_sentry.hpp>
#include <literal/detail/leaf_error_handler.hpp>
#include <literal/convert.hpp>

#include <boost/spirit/home/x3.hpp>

#include <boost/leaf.hpp>

#include <fmt/format.h>

#include <cmath>
#include <limits>

#include <iostream>
#include <iomanip>

namespace parser {

namespace x3 = boost::spirit::x3;
namespace leaf = boost::leaf;

using based_integer_base_tag = struct {};

namespace detail {

// BNF base ::= integer
struct based_base_specifier_parser : x3::parser<based_base_specifier_parser> {
    // an extra context is used to write the base specifier attribute into
    using attribute_type = x3::unused_type;
    static bool const has_attribute = false;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, x3::unused_type,
               x3::unused_type) const
    {
        auto const begin = first;

        std::string base_literal_str;
        bool const parse_ok = x3::parse(first, last, char_parser::dec_integer, base_literal_str);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return leaf::try_catch(
            [&] {
                // base specifier is always decimal
                static constexpr auto const base10 = 10U;

                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // LEAF - from_chars() may fail
                auto const base_result =
                    convert::detail::as_integral_integer<unsigned>(base10, base_literal_str);
                // store the parsed base for use at based_{real, integer} later
                x3::get<based_integer_base_tag>(ctx) = base_result;

                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
    }

private:
    static bool supported_base(unsigned base)
    {
        return base == 2U || base == 8U || base == 10U || base == 16U;
    }
};

static based_base_specifier_parser const based_base_specifier = {};

// BNF: based_literal := based_integer [ . based_integer ] [ exponent ]
struct based_integer_parser : x3::parser<based_integer_parser> {
    using attribute_type = ast::integer_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, x3::unused_type,
               attribute_type& attribute) const
    {
        auto const begin = first;

        // Note: the base has been initialized by outer rule before
        attribute.base = x3::get<based_integer_base_tag>(ctx);

        auto const based_integer = char_parser::based_integer<IteratorT>(attribute.base);
        using detail::unsigned_exp;

        auto const grammar =  // use lexeme[] from outer parser
            based_integer >> '#' >> -unsigned_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return leaf::try_catch(
            [&] {
                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // LEAF - from_chars() or power() may fail
                attribute.value = convert::integer<attribute_type::value_type>(attribute);
                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
    }
};

static based_integer_parser const based_integer = {};

struct based_real_parser : x3::parser<based_real_parser> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, x3::unused_type,
               attribute_type& attribute) const
    {
        auto const begin = first;

        // Note: the base has been initialized by outer rule before
        attribute.base = x3::get<based_integer_base_tag>(ctx);

        auto const based_integer = char_parser::based_integer<IteratorT>(attribute.base);
        using detail::signed_exp;

        auto const grammar =  // use lexeme[] from outer parser
            based_integer >> '.' >> x3::expect[based_integer] >> '#' >> -signed_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return leaf::try_catch(
            [&] {
                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // LEAF - from_chars() or power() may fail
                attribute.value = convert::real<attribute_type::value_type>(attribute);
                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
    }
};

static based_real_parser const based_real = {};

}  // namespace detail

struct based_literal_parser : x3::parser<based_literal_parser> {
    using attribute_type = ast::based_literal;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, x3::unused_type,
               attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using detail::based_base_specifier;
        using detail::based_integer;
        using detail::based_real;

        // store the parsed base (specifier) for use at based_{real, integer}, by introducing an
        // extra context
        // clang-format off
        auto const grammar = x3::with<based_integer_base_tag>(unsigned{} /* base */)[
            x3::lexeme[
                based_base_specifier >> '#' >> (based_real | based_integer)
            ]
        ];
        // clang-format on

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return true;
    }
};

static based_literal_parser const based_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::detail::based_base_specifier_parser> {
    using result_type = std::string;
    std::string operator()(
        [[maybe_unused]] ::parser::detail::based_base_specifier_parser const&) const
    {
        return "based literal (base)";
    }
};

template <>
struct get_info<::parser::detail::based_integer_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::detail::based_integer_parser const&) const
    {
        return "based literal (integer)";
    }
};

template <>
struct get_info<::parser::detail::based_real_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::detail::based_real_parser const&) const
    {
        return "based literal (real)";
    }
};

template <>
struct get_info<::parser::based_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::based_literal_parser const&) const
    {
        return "based literal";
    }
};

}  // namespace boost::spirit::x3
