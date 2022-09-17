//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <literal/ast.hpp>
#include <literal/parser/based_literal.hpp>
#include <literal/parser/decimal_literal.hpp>

namespace parser {

namespace x3 = boost::spirit::x3;

// BNF: abstract_literal ::= decimal_literal | based_literal
// Note: {decimal, based}_literal's AST nodes does have same memory layout!
struct abstract_literal_parser : x3::parser<abstract_literal_parser> {
    using attribute_type = ast::abstract_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, ContextT const& ctx,
               [[maybe_unused]]RContextT const&, attribute_type& attribute) const
    {
        skip_over(first, last, ctx);

        auto const grammar = x3::lexeme [
            based_literal | decimal_literal
        ];

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            return false;
        }

        return true;
    }
};

static abstract_literal_parser const abstract_literal = {};

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::abstract_literal_parser> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::abstract_literal_parser const&) const
    {
        return "based or decimal (abstract) literal";
    }
};

}  // namespace boost::spirit::x3
