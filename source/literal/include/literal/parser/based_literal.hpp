//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/parser/error_handler.hpp>
#include <literal/convert/leaf_error_handler.hpp>
#include <literal/convert/convert.hpp>

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

/// The Spirit X3 context tag for based integer literal's base specifier
using based_integer_base_tag = struct {};

/// Check, if a base is in range of [2 ... 36]
/// These restrictions come from the limited ASCII charset respectively
/// std::from_chars(), std::strtol() etc.
inline bool valid_base(unsigned base) {
    return (2U <= base && base <= 36U);
}

inline bool vhdl_supported_base(unsigned base) {
    return base == 2U || base == 8U || base == 10U || base == 16U;
}

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
        LEAF_ERROR_TRACE;

        auto const begin = first;

        std::string base_literal_str;
        bool const parse_ok = x3::parse(first, last, char_parser::dec_integer, base_literal_str);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        // convert parse result into base specifier and use `with` directive to
        // inject this value into the Spirit X3 context. Base specifier is required to select
        // correct character based_{real, integer} parser later.
        return leaf::try_catch(
            [&] {
                // base specifier is always decimal
                static constexpr auto const base10 = 10U;

                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // from_chars() may fail
                auto const base_result =
                    convert::detail::as_integral_integer<unsigned>(base10, base_literal_str);

                x3::get<based_integer_base_tag>(ctx) = base_result;

                // Note: here, no check for a valid base can take place, because this parser
                // is called also for other rules due to parser alternatives (namely decimal literal).

                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
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
        LEAF_ERROR_TRACE;

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

#if defined(USE_CONVERT)
        return leaf::try_catch(
            [&] {
                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // LEAF - from_chars() or power() may fail
                attribute.value = convert::integer<attribute_type::value_type>(attribute);
                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
#else
        return true;
#endif
    }
};

static based_integer_parser const based_integer = {};

struct based_real_parser : x3::parser<based_real_parser> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, x3::unused_type,
               attribute_type& attribute) const
    {
        LEAF_ERROR_TRACE;

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

#if defined(USE_CONVERT)
        return leaf::try_catch(
            [&] {
                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                // LEAF - from_chars() or power() may fail
                attribute.value = convert::real<attribute_type::value_type>(attribute);
                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
#else
        return true;
#endif
    }
};

static based_real_parser const based_real = {};

}  // namespace detail

// BNF: based_literal ::= base # based_integer [ . based_integer ] # [ exponent ]
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

        //std::cout << "based_literal_parser: '" <<std::string_view(first, first + 12) << "'\n";

        auto const check_base = x3::rule<struct _>{ "valid base specifier" } = x3::eps[
            ([&](auto&& ctx){
                _pass(ctx) = valid_base(x3::get<based_integer_base_tag>(ctx));
            })];

        // This parser is tricky. The base of the literal determines the following, actual literal
        // parser regarding the charset. Therefore the parser has three parts. The result of the base
        // parser is stored in the (local) context and fetched later. In between is the check for a valid
        // base range.

        // TODO The expect directive for valid base specifier works, but the error location indicator
        // show to the (to late) iterator position. Hence the error message isn't such intuitive.

        // clang-format off
        auto const grammar = x3::with<based_integer_base_tag>(unsigned{} /* base specifier */)[
            x3::lexeme[
                based_base_specifier >> '#' >> x3::expect[ check_base ] >> (based_real | based_integer)
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
