//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/parser/integer_parser.hpp>

#include <boost/spirit/home/x3.hpp>

#include <cstdint>
#include <type_traits>

namespace parser {

namespace x3 = boost::spirit::x3;

// FixMe: Specialize, that the base can't be changed!
template<unsigned Base = 10U, typename AttributeT = std::uint32_t>
struct literal_base_parser : x3::parser<literal_base_parser<Base, AttributeT>> {

    // base must be of type integer
    static_assert(std::is_integral<AttributeT>::value, "Integral required");
    // only positive bases are defined
    static_assert(std::is_unsigned<AttributeT>::value, "Base integral type must be unsigned");

    using attribute_type = AttributeT;

    template<typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
        [[maybe_unused]] RContextT const& rctx, AttributeT& base_attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;
        static auto const parser = parser::integer_parser<Base, attribute_type>{};
        bool const parse_ok = x3::parse(first, last, parser, base_attribute);

        if (!parse_ok || !supported_base(base_attribute)) {
            first = begin;
            return false;
        }
        return true;
    }

    bool supported_base(attribute_type base) const {
        if (base == 2 || base == 8 || base == 10 || base == 16) {
            return true;
        }
        return false;
    }
};

} // namespace parser
