//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/util/x3_lazy.hpp>

#include <boost/spirit/home/x3.hpp>

#include <range/v3/view/filter.hpp>
#include <range/v3/range/conversion.hpp>
#include <charconv>

#include <iostream>

namespace parser {

namespace x3 = boost::spirit::x3;

// BNF: bit_string_literal ::= base_specifier " [ bit_value ] "
// FixMe: ATTENTION:  In VHDL-1993 hexadecimal bit-string literals always contain a
// multiple of 4 bits, and octal ones a multiple of 3 bits. VHDL-2008 they may have:
// - an explicit width,
// - declared as signed or unsigned (e.g. UB, UX, SB, SX,...)
// - include meta-values ('U', 'X', etc.)
struct bit_string_literal_parser : x3::parser<bit_string_literal_parser> {
    using attribute_type = ast::bit_string_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
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

        std::error_code ec;
        attribute.value = convert::bit_string_literal<attribute_type::value_type>(attribute, ec);

        if (ec) {
            std::cerr << "error: " << ec.message() << " '" << attribute << "'\n";
            first = begin;
            return false;
        }

        return true;
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
