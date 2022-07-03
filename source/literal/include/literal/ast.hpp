//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/optional.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace ast {

namespace x3 = boost::spirit::x3;

template <typename... Types>
using variant = x3::variant<Types...>;
template <typename T>
using optional = boost::optional<T>;

struct real_type : x3::position_tagged {
    // boost::iterator_range<Iter> later on
    std::string integer;     // base: 2,8,10,16
    std::string fractional;  // base: 2,8,10,16
    optional<std::int32_t> exponent;
};

struct integer_type : x3::position_tagged {
    std::string integer;               // base: 2,8,10,16
    optional<std::uint32_t> exponent;  // positive only!
};

struct based_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    std::uint32_t base;
    num_type num;
    // e.g. https://coliru.stacked-crooked.com/a/652a879e37b4ea37
    // std::variant<RealT, IntT> const value() // lazy numeric conversion
};

struct decimal_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    std::uint32_t base;
    num_type num;
    // std::variant<RealT, IntT> const value() // lazy numeric conversion
};

// Note: The literal representation is needed, at the latest with VHDL 2008
// where also literals like 12UX"F-" are possible.
struct bit_string_literal : x3::position_tagged {
    std::uint32_t base;
    std::string integer;  // base: 2,8,10,16
};

using abstract_literal = variant<based_literal, decimal_literal>;
// using literal = variant<numeric_literal, enumeration_literal, string_literal,
// bit_string_literal>;
using literal = variant<abstract_literal, bit_string_literal>;
using literals = std::vector<literal>;

std::ostream& operator<<(std::ostream& os, ast::real_type const& r);
std::ostream& operator<<(std::ostream& os, ast::integer_type const& i);
std::ostream& operator<<(std::ostream& os, ast::based_literal const& l);
std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& l);
std::ostream& operator<<(std::ostream& os, ast::abstract_literal const& l);
std::ostream& operator<<(std::ostream& os, ast::bit_string_literal const& l);
std::ostream& operator<<(std::ostream& os, ast::literal const& l);

}  // namespace ast

BOOST_FUSION_ADAPT_STRUCT(ast::real_type, integer, fractional, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::integer_type, integer, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::based_literal, base, num)
BOOST_FUSION_ADAPT_STRUCT(ast::decimal_literal, num, base)
BOOST_FUSION_ADAPT_STRUCT(ast::bit_string_literal, base, integer)
