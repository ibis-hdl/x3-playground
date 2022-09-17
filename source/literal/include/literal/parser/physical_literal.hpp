//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <literal/ast.hpp>
#include <literal/parser/abstract_literal.hpp>

namespace parser {

namespace x3 = boost::spirit::x3;

// BNF: physical_literal ::= [ abstract_literal ] unit_name
//
// ATTENTION:In the BNF, the abstract_literal is optional. This may lead to a standalone
// unit_name, with a default-constructed abstract_literal or more concrete based_literal
// (and with base = 0) and an arbitrary unit_name - depending on the following lexemes.
// This must be taken into account when implementing the secondary_unit_declaration!
struct physical_literal_parser : x3::parser<physical_literal_parser> {
    using attribute_type = ast::physical_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               [[maybe_unused]]RContextT const&, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);

        // Note, the LRM doesn't specify the allowed characters, hence it's assumed
        // that it follows the natural conventions.
        auto const unit_name = x3::rule<struct unit_name_class, std::string>{ "unit name" } =
            x3::lexeme[ +x3::alpha ]
            ;

        auto const grammar = x3::lexeme [
            abstract_literal >> unit_name
        ];

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            return false;
        }

        return true;
    }
};

static physical_literal_parser const physical_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::physical_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::physical_literal_parser const&) const
    {
        return "physical literal";
    }
};

}  // namespace boost::spirit::x3
