//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once


#include <literal/ast.hpp>
#include <literal/parser/decimal_literal.hpp>
#include <literal/parser/bit_string_literal.hpp>
#include <literal/parser/based_literal.hpp>
#include <literal/parser/identifier.hpp>
#include <literal/parser/string_literal.hpp>
#include <literal/parser/comment.hpp>
#include <literal/parser/error_handler.hpp>
#include <literal/parser/parser_id.hpp>

#include <boost/spirit/home/x3.hpp>

#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace parser {

// clang-format off

// BNF: abstract_literal ::= decimal_literal | based_literal
// Note: {decimal, based}_literal's AST nodes does have same memory layout!
auto const abstract_literal = x3::rule<struct abstract_literal_class, ast::abstract_literal>{ "based or decimal abstract literal" } =
    based_literal | decimal_literal
    ;

// Note, the LRM doesn't specify the allowed characters, hence it's assumed
// that it follows the natural conventions.
auto const unit_name = x3::rule<struct unit_name_class, std::string>{ "unit name" } =
    x3::lexeme[ +x3::alpha ]
    ;

// BNF: physical_literal ::= [ abstract_literal ] unit_name
// ATTENTION:In the BNF, the abstract_literal is optional. This may lead to a standalone
// unit_name, with a default-constructed abstract_literal or more concrete based_literal
// (and with base = 0) and an arbitrary unit_name - depending on the following lexemes.
// This must be taken into account when implementing the secondary_unit_declaration!
auto const physical_literal = x3::rule<struct physical_literal_class, ast::physical_literal>{ "physical literal" } =
    abstract_literal >> unit_name;
    ;

// BNF: numeric_literal ::= abstract_literal | physical_literal
auto const numeric_literal = x3::rule<struct numeric_literal_class, ast::numeric_literal>{ "numeric literal" } =
    physical_literal | abstract_literal // order matters
    ;

// BNF: character_literal ::= ' graphic_character '
auto const character_literal = x3::rule<character_literal_class, ast::character_literal>{ "character literal" } =
    x3::lexeme ["\'" >> x3::expect[( graphic_character - "\'" ) | x3::char_("\'")] >> "\'" ]
    ;

// BNF: enumeration_literal ::= identifier | character_literal
auto const enumeration_literal = x3::rule<struct enumeration_literal_class, ast::enumeration_literal>{ "enumeration literal" } =
    identifier | character_literal
    ;

// BNF: literal ::= numeric_literal | enumeration_literal | string_literal | bit_string_literal | null
auto const literal = x3::rule<struct literal_class, ast::literal>{ "literal" } = // order matters
    NULL_ | enumeration_literal | string_literal | bit_string_literal | numeric_literal
    ;

auto const literal_rule = x3::rule<literal_rule_class, ast::literal>{ "literal" } =
    x3::lit("X") > ":=" > literal > ';'
    ;

auto const grammar = x3::rule<grammar_class, ast::literals>{ "grammar" } =
    x3::skip(x3::space | comment)[
        *literal_rule
    ];

// clang-format on

}  // namespace parser
