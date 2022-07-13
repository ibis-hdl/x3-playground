//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>

#include <boost/spirit/home/x3.hpp>

#include <cmath>
#include <limits>

#include <iostream>
#include <iomanip>

namespace parser {

namespace x3 = boost::spirit::x3;

using based_integer_base_tag = struct {};

namespace detail {

// BNF base ::= integer
struct based_base_specifier_parser : x3::parser<based_base_specifier_parser> {

    // an extra context is used to write the base specifier attribute into
    using attribute_type = x3::unused_type;
    static bool const has_attribute = false;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               x3::unused_type, x3::unused_type) const
    {
        auto const begin = first;

        std::string base_literal_str;
        bool const parse_ok = x3::parse(first, last, char_parser::dec_integer, base_literal_str);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec;
        // base specifier is always decimal
        auto const base_result = convert::detail::as_integral_integer<unsigned>(10U, base_literal_str, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << base_literal_str << "'\n";
            return false;
        }

        // store the parsed base for use at based_{real, integer} later
        x3::get<based_integer_base_tag>(ctx) = base_result;

        if (!supported_base(base_result)) {
            first = begin;
            return false;
        }

        return true;
    }

private:
    static bool supported_base(unsigned base)
    {
        if (base == 2 || base == 8 || base == 10 || base == 16) {
            return true;
        }
        return false;
    }
};

static based_base_specifier_parser const based_base_specifier = {};

// BNF: based_literal := based_integer [ . based_integer ] [ exponent ]
struct based_integer_parser : x3::parser<based_integer_parser> {
    using attribute_type = ast::integer_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        auto const begin = first;

        // Note: the base has been initialized by outer rule before
        attribute.base = x3::get<based_integer_base_tag>(ctx);
        unsigned const base = attribute.base;

        auto const based_integer = char_parser::based_integer<IteratorT>(base);
        using detail::unsigned_exp;

        auto const grammar =  // use lexeme[] from outer parser
            based_integer >> '#' >> -unsigned_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec{};
        attribute.value = convert::integer<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static based_integer_parser const based_integer = {};

struct based_real_parser : x3::parser<based_real_parser> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
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

        std::error_code ec{};
        attribute.value = convert::real<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static based_real_parser const based_real = {};

} // namespace detail

struct based_literal_parser : x3::parser<based_literal_parser> {

    using attribute_type = ast::based_literal;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using detail::based_base_specifier;
        using detail::based_real;
        using detail::based_integer;

        // store the parsed base (specifier) for use at based_{real, integer}, by introducing an
        // extra context
        // clang-format off
        auto const grammar = x3::with<based_integer_base_tag>(unsigned{} /* base */)[  //
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

} // namespace parser
