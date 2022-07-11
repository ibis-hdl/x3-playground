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

namespace detail {

// BNF base ::= integer
struct based_integer_base_parser : x3::parser<based_integer_base_parser> {

    using attribute_type = std::uint32_t;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& base_attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;
        std::string base_literal_str;
        bool const parse_ok = x3::parse(first, last, char_parser::dec_integer, base_literal_str);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec;
        auto const base_result = convert::detail::as_unsigned<attribute_type>(base_literal_str, 10U, ec);

        if (!ec) {
            base_attribute = base_result;
        }
        else {
            std::cerr << "error: " << ec.message() << " '" << base_literal_str << "'\n";
            return false;
        }

        if (!supported_base(base_attribute)) {
            first = begin;
            return false;
        }

        return true;
    }

private:
    static bool supported_base(attribute_type base)
    {
        if (base == 2 || base == 8 || base == 10 || base == 16) {
            return true;
        }
        return false;
    }
};

static based_integer_base_parser const based_integer_base = {};

// BNF: based_literal := based_integer [ . based_integer ] [ exponent ]
struct based_integer_parser : x3::parser<based_integer_parser> {
    using attribute_type = ast::integer_type;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_integer; // FIXME, based_integer
        using detail::unsigned_exp;

        auto const grammar =
            dec_integer >> '#' >> -unsigned_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec; // TODO
        unsigned base = 10U; // FixMe
        attribute.value = convert::integer<attribute_type::value_type>(attribute, base, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static based_integer_parser const based_integer = {};

struct based_integer_real_parser : x3::parser<based_integer_real_parser> {
    using attribute_type = ast::real_type;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using char_parser::dec_integer;  // FIXME, based_integer
        using detail::signed_exp;

        auto const grammar =  //
            dec_integer >> '.' >> x3::expect[dec_integer] >> '#' >> -signed_exp;

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        std::error_code ec; // TODO
        unsigned base = 10U; // FixMe
        attribute.value = convert::real<attribute_type::value_type>(attribute, base, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
    }
};

static based_integer_real_parser const based_real = {};

} // namespace detail

struct based_literal_parser : x3::parser<based_literal_parser> {

    using attribute_type = ast::based_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;

        using detail::based_integer_base;
        using detail::based_real;
        using detail::based_integer;

        auto const grammar = //
            based_integer_base >> '#' >> (based_real | based_integer);

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
