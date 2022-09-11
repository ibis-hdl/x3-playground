//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/spirit/home/x3.hpp>

#include <iostream>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace detail {

struct distinct_directive {

    template <typename Parser>
    constexpr auto operator()(Parser&& parser) const {
        return x3::lexeme[
            x3::no_case[ std::forward<Parser>(parser) ] >> !(x3::iso8859_1::alnum | '_')
        ];
    }
};

static const distinct_directive distinct = {};

static x3::symbols<> const keywords(
    // clang-format off
    {
        "abs", "access", "after", "alias", "all", "and", "architecture",
        "array", "assert", "attribute", "begin", "block", "body", "buffer",
        "bus", "case", "component", "configuration", "constant", "disconnect",
        "downto", "else", "elsif", "end", "entity", "exit", "file", "for",
        "function", "generate", "generic", "group", "guarded", "if", "impure",
        "in", "inertial", "inout", "is", "label",  "library", "linkage",
        "literal", "loop", "map", "mod", "nand", "new", "next", "nor", "not",
        "null", "of", "on",  "open", "or", "others", "out", "package", "port",
        "postponed", "procedure", "process", "pure", "range", "record",
        "register", "reject", "rem", "report", "return", "rol", "ror",
        "select", "severity", "signal", "shared", "sla", "sll", "sra", "srl",
        "subtype", "then", "to", "transport", "type", "unaffected", "units",
        "until", "use", "variable", "wait", "when", "while", "with", "xnor",
        "xor"
    },
    "keyword"
    // clang-format on
);

static auto const keyword = distinct(keywords);

static auto const feasible_identifier =
    // clang-format off
    x3::raw[ x3::lexeme [
           x3::alpha
        >> !x3::char_('"') // reject bit_string_literal
        >> *( x3::alnum | '_' )
    ]];
    // clang-format on


static auto const basic_identifier = x3::rule<struct _, ast::identifier> { "basic identifier" } =
    feasible_identifier - keyword;

} // end detail

static auto const identifier = detail::basic_identifier;

static auto const primary_unit_name = identifier;

// simplify keyword handling, no extra AST node
static auto const NULL_ = x3::rule<struct _, ast::identifier> { "NULL" } = detail::distinct("null") >> x3::attr("kw:NULL");

} // namespace parser
