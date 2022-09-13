//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <literal/ast.hpp>
#include <literal/parser/graphic_character.hpp>

#include <fmt/format.h>

namespace parser {

namespace x3 = boost::spirit::x3;

// BNF: string_literal ::= " { graphic_character } "
struct string_literal_parser : x3::parser<string_literal_parser> {
    using attribute_type = ast::string_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx, 
               [[maybe_unused]]RContextT const&, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);

        auto const string_literal_charset = [](char delim) {
            return x3::rule<struct _, std::string>{ "string literal charset" } =
            *(   ( graphic_character - x3::char_(delim)  )
            | ( x3::char_(delim) >> x3::char_(delim) )
            )
            ;
        };

        auto const string_literal = [&](char delim) {
            return x3::rule<struct _, std::string>{ "string literal" } =
                x3::lit(delim) >> x3::raw[string_literal_charset(delim)] >> x3::lit(delim)
            ;
        };

        auto const grammar = x3::lexeme [
            string_literal('"') | string_literal('%')
        ];

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            //std::cout << "string_literal_parser failed\n";
            return false;
        }

        return true;
    }
};

static string_literal_parser const string_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::string_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::string_literal_parser const&) const
    {
        return "string literal";
    }
};

}  // namespace boost::spirit::x3
