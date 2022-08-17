//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

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

        auto load = leaf::on_error([&]{ // lazy for call of x3::what()
            return leaf::e_convert_parser_context{x3::what(*this), first, begin}; });

        return leaf::try_catch(
            [&] {
                attribute.value =
                    convert::bit_string_literal<attribute_type::value_type>(attribute);
                return true;
            },
            [](std::error_code const& ec, leaf::e_convert_parser_context<IteratorT>& parser_ctx,
                leaf::e_api_function const* api_fcn, leaf::e_fp_exception const* fp_exception,
                leaf::e_position_iterator<IteratorT> const* e_iter) {

                std::cerr << "LEAF handler #1 called.\n";
                // LEAF - e.g. safe_{mu,add}<IntT>
                if(api_fcn) {
                    std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn->value, ec.message());
                }
                if(fp_exception) {
                    std::cerr << fmt::format("LEAF: Error during FP operation '{}': {}\n", fp_exception->as_string(), ec.message());
                }
                parser_ctx.unroll();
                // The e_iter is set by from_chars API and points to erroneous position, otherwise take the iterator
                // position from global VHDL parser.
                IteratorT iter = (e_iter) ? e_iter->value : parser_ctx.iter();

                leaf::throw_exception( // --
                    convert::numeric_failure<IteratorT>(
                        // notation x3::expectation_failure(where, which, what)
                        iter, parser_ctx.which(), ec.message()
                ));
                return false;
            },
            [](std::exception const& e, leaf::e_convert_parser_context<IteratorT>& parser_ctx) {
                std::cerr << "LEAF handler #2 called.\n";
                parser_ctx.unroll();
                leaf::throw_exception( // --
                    convert::numeric_failure<IteratorT>(
                        // notation x3::expectation_failure(where, which, what)
                        parser_ctx.iter(), parser_ctx.which(), e.what()
                ));
                return false;
            },
            // FIXME gets never called
            [](leaf::error_info const& unmatched, leaf::e_convert_parser_context<IteratorT>& parser_ctx)
            {
                std::cerr << "LEAF: Unknown failure detected:\n" << unmatched;
                std::cerr << "\n  try to recover\n";
                parser_ctx.unroll();
                leaf::throw_exception( // --
                    convert::numeric_failure<IteratorT>(
                        // notation x3::expectation_failure(where, which, what)
                        parser_ctx.iter(), parser_ctx.which(), unmatched.exception()->what()
                ));
                return false;
            },            
            [](leaf::error_info const& unmatched)
            {
                std::cerr << "LEAF: Unknown failure detected:\n" << unmatched;
                // TODO Here is no parser context available, so recovery and useful error handling isn't 
                // possible anymore. This is considered as serious bug.
                abort();
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
