//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <range/v3/view/filter.hpp>

#include <type_traits>
#include <string>
#include <string_view>

namespace parser {

namespace x3 = boost::spirit::x3;

template <unsigned Base, typename AttributeT>
struct integer_parser : x3::parser<integer_parser<Base, AttributeT>> {
    static_assert(std::is_integral<AttributeT>::value, "Integral required.");

    using attribute_type = AttributeT;

    auto constexpr parser() const
    {
        if constexpr (std::is_unsigned<AttributeT>::value) {
            return x3::uint_parser<AttributeT, Base>{};
        }
        else {
            return x3::int_parser<AttributeT, Base>{};
        }
    }

    template <typename IteratorT>
    static auto delimited_view(IteratorT first, IteratorT last)
    {
        namespace views = ranges::views;
        std::string_view const literal_sv{ first, last };
        return literal_sv | views::filter([](char chr) { return chr != '_'; });
    }

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, AttributeT& integer_attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;
        auto view = delimited_view(first, last);
        bool const parse_ok =
            x3::parse(std::begin(view), std::end(view), parser() >> x3::eoi, integer_attribute);

        if (!parse_ok) {
            first = begin;  // rewind iter
            return false;
        }
        return true;
    }
};

}  // namespace parser
