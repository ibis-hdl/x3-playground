//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/parser/error_handler.hpp>
#include <literal/parser/util/x3_lazy.hpp>
#include <literal/convert_error.hpp>
#include <literal/detail/leaf_error_handler.hpp>
#include <literal/convert.hpp>

#include <boost/spirit/home/x3.hpp>

#include <boost/leaf.hpp>

#include <range/v3/view/filter.hpp>
#include <range/v3/range/conversion.hpp>
#include <charconv>

#include <fmt/format.h>

#include <iostream>

namespace parser {

namespace x3 = boost::spirit::x3;
namespace leaf = boost::leaf;

// BNF: bit_string_literal ::= base_specifier " [ bit_value ] "
// ATTENTION:  In VHDL-1993 hexadecimal bit-string literals always contain a
// multiple of 4 bits, and octal ones a multiple of 3 bits. VHDL-2008 they may have:
// - an explicit width,
// - declared as signed or unsigned (e.g. UB, UX, SB, SX,...)
// - include meta-values ('U', 'X', etc.)
struct bit_string_literal_parser : x3::parser<bit_string_literal_parser> {
    using attribute_type = ast::bit_string_literal;

    template <typename IteratorT, typename ContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               x3::unused_type, attribute_type& attribute) const
    {
        using rule_type = x3::any_parser<IteratorT, std::string>;
        using char_parser::bin_integer;
        using char_parser::hex_integer;
        using char_parser::oct_integer;
        using x3e::do_lazy;
        using x3e::set_lazy;

        skip_over(first, last, ctx);

        auto const begin = first;

        // clang-format off
        x3::symbols<rule_type> const bit_value_parser ({
            {"b", bin_integer},
            {"o", oct_integer},
            {"x", hex_integer},
        }, "bit value");

        auto const bit_value = x3::rule<struct _, rule_type>{ "based digits" } =
            x3::no_case[ -bit_value_parser ];

        auto const base_specifier = x3::rule<struct _, std::uint32_t>{ "base specifier" } =
            x3::no_case[ base_id ];

        // clang-format off
        auto const grammar = x3::rule<struct _, attribute_type, true>{ "bit string literal" } =
            x3::with<rule_type>( rule_type{} )[
                x3::lexeme[
                       &set_lazy<rule_type>[ bit_value ] >> base_specifier
                    >> '"' >> do_lazy<rule_type> >> '"'
                ]
            ];
        // clang-format on

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;
            return false;
        }

        return leaf::try_catch(
            [&] {
                attribute.value =
                    convert::bit_string_literal<attribute_type::value_type>(attribute);
                return true;
            },
            [&](std::error_code const& ec, leaf::e_api_function const& api_fcn,
                leaf::e_position_iterator<IteratorT> const& e_iter) {
                // LEAF - from_chars()
                std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn.value,
                                         ec.message());
                boost::throw_exception(convert::numeric_failure<IteratorT>(
                    // ex.where() - ex.which() - ex.what()
                    e_iter, x3::what(*this), ec.message()));
                first = begin;
                return false;
            },
            [&](std::error_code const& ec, leaf::e_api_function const& api_fcn,
                leaf::e_fp_exception fp_exception) {
                // LEAF - safe_{mul,add}<RealT>
                std::cerr << fmt::format("LEAF: Error in api function '{}': {} ({})\n",
                                         api_fcn.value, ec.message(), fp_exception.as_string());
                boost::throw_exception(convert::numeric_failure<IteratorT>(
                    // ex.where() - ex.which() - ex.what()
                    begin, x3::what(*this), ec.message()));
                first = begin;
                return false;
            },
            [&](std::error_code const& ec, leaf::e_api_function const& api_fcn) {
                // LEAF - safe_{mu,add}<IntT>
                std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn.value,
                                         ec.message());
                boost::throw_exception(convert::numeric_failure<IteratorT>(
                    // ex.where() - ex.which() - ex.what()
                    begin, x3::what(*this), ec.message()));
                first = begin;
                return false;
            },
            [&](std::error_code const& ec) {
                // LEAF
                std::cerr << fmt::format("LEAF: Error: '{}'\n", ec.message());
                boost::throw_exception(convert::numeric_failure<IteratorT>(
                    // ex.where() - ex.which() - ex.what()
                    begin, x3::what(*this), ec.message()));
                first = begin;
                return false;
            },
            [&](std::exception const& e) {
                std::cerr << fmt::format("LEAF: Caught exception: '{}'\n", e.what());
                boost::throw_exception(convert::numeric_failure<IteratorT>(
                    // ex.where() - ex.which() - ex.what()
                    begin, x3::what(*this), e.what()));
                first = begin;
                return false;
            });
    }

private:
    // clang-format off
    x3::symbols<std::uint32_t> const base_id = x3::symbols<std::uint32_t>({
        { "b", 2 },
        { "o", 8 },
        { "x", 16 },
    }, "base id");
    // clang-format on
};

static bit_string_literal_parser const bit_string_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::bit_string_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::bit_string_literal_parser const&) const
    {
        return "bit string literal";
    }
};

}  // namespace boost::spirit::x3
