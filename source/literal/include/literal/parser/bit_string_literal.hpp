//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/parser/error_handler.hpp>
#include <literal/convert/convert.hpp>
#include <literal/convert/leaf_error_handler.hpp>

#include <boost/spirit/home/x3.hpp>
#include <literal/parser/util/x3_lazy.hpp>

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
        using char_parser::bin_digits;
        using char_parser::hex_digits;
        using char_parser::oct_digits;
        using x3e::do_lazy;
        using x3e::set_lazy;

        skip_over(first, last, ctx);

        auto const begin = first;

        // clang-format off
        x3::symbols<rule_type> const bit_value_parser ({
            {"b", bin_digits},
            {"o", oct_digits},
            {"x", hex_digits},
        }, "bit value");

        auto const bit_value = x3::rule<struct _, rule_type>{ "based digits" } =
            x3::no_case[ bit_value_parser ];

        auto const base_specifier = x3::rule<struct _, std::uint32_t>{ "base specifier" } =
            x3::no_case[ base_id ];

        // clang-format off
        auto const grammar = x3::rule<struct _, attribute_type, true>{ "bit string literal" } =
            x3::with<rule_type>( rule_type{} )[
                x3::lexeme[
                       &set_lazy<rule_type>[ bit_value ] >> base_specifier
                    >> '"' >> -do_lazy<rule_type> >> '"'
                ]
            ];
        // clang-format on

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            return false;
        }
#if defined(USE_IN_PARSER_CONVERT)
        return leaf::try_catch(
            [&] {
                auto load = leaf::on_error(leaf::e_x3_parser_context{*this, first, begin});

                attribute.value =
                    convert::bit_string_literal<attribute_type::value_type>(attribute);
                return true;
            },
            convert::leaf_error_handlers<IteratorT>);
#else
        return true;
#endif
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
