//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <variant>
#include <vector>

namespace ast {

namespace x3 = boost::spirit::x3;

template <typename... Types>
using variant = x3::variant<Types...>;

template <typename T>
using optional = boost::optional<T>;

struct real_type : x3::position_tagged {
    unsigned base{};
    std::string integer;
    std::string fractional;
    std::string exponent;
    // numeric representation
    using value_type = double;
    std::optional<value_type> value;
};

struct integer_type : x3::position_tagged {
    unsigned base{};
    std::string integer;
    std::string exponent;  // positive only!
    // numeric representation
    using value_type = std::uint32_t;
    std::optional<value_type> value;
};

struct based_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    num_type num;
};

struct decimal_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    num_type num;
};

using abstract_literal = variant<based_literal, decimal_literal>;

// Note: The literal representation is needed, at the latest with VHDL-2008
// where also literals like 12UX"F-" are possible.
struct bit_string_literal : x3::position_tagged {
    std::uint32_t base;
    std::string literal;
    // numeric representation
    using value_type = std::uint32_t;
    std::optional<value_type> value;
};

struct identifier : x3::position_tagged {
    std::string name;
};

struct physical_literal : x3::position_tagged {
    abstract_literal literal;
    std::string unit_name;
};

using numeric_literal = variant<abstract_literal, physical_literal>;

struct character_literal : x3::position_tagged {
    char literal;
};

using enumeration_literal = variant<identifier, character_literal>;

// using literal = variant<numeric_literal, enumeration_literal, string_literal,
// bit_string_literal>;
using literal = variant<abstract_literal, bit_string_literal>;
using literals = std::vector<literal>;

std::ostream& operator<<(std::ostream& os, ast::real_type const& real);
std::ostream& operator<<(std::ostream& os, ast::integer_type const& int_);
std::ostream& operator<<(std::ostream& os, ast::based_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::abstract_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::bit_string_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::identifier const& ident);
std::ostream& operator<<(std::ostream& os, ast::physical_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::numeric_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::character_literal const& literal);
std::ostream& operator<<(std::ostream& os, ast::enumeration_literal const& literal);

}  // namespace ast

BOOST_FUSION_ADAPT_STRUCT(ast::real_type, integer, fractional, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::integer_type, integer, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::based_literal, num)
BOOST_FUSION_ADAPT_STRUCT(ast::decimal_literal, num)
BOOST_FUSION_ADAPT_STRUCT(ast::bit_string_literal, base, literal)
BOOST_FUSION_ADAPT_STRUCT(ast::identifier, name)
BOOST_FUSION_ADAPT_STRUCT(ast::physical_literal, literal, unit_name)
BOOST_FUSION_ADAPT_STRUCT(ast::character_literal, literal)
