//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <literal/ast.hpp>
#include <literal/parser/graphic_character.hpp>

namespace parser {

namespace x3 = boost::spirit::x3;

// BNF: character_literal ::= ' graphic_character '
struct character_literal_parser : x3::parser<character_literal_parser> {
    using attribute_type = ast::character_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               [[maybe_unused]]RContextT const&, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);

        auto const grammar = x3::lexeme [
            "\'" >> x3::expect[( graphic_character - "\'" ) | x3::char_("\'")] >> "\'"
        ];

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            return false;
        }

        return true;
    }
};

static character_literal_parser const character_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::character_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::character_literal_parser const&) const
    {
        return "character literal";
    }
};

}  // namespace boost::spirit::x3
