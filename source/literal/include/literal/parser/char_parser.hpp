//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/copy_n.hpp>

#include <literal/convert/detail/constraint_types.hpp>

#include <string_view>
#include <string>
#include <cassert>
#include <iostream>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace char_parser {

/// Check, if a base is in range of [2 ... 36]
/// These restrictions come from the limited ASCII charset respectively
/// std::from_chars(), std::strtol() etc.
inline bool valid_base(unsigned base) {
    return (2U <= base && base <= 36U);
}

#if 0
// FixMe: Limit digits, see [Misunderstanding repeat directive - it should fail, but doesn't](
// https://stackoverflow.com/questions/72915071/misunderstanding-repeat-directive-it-should-fail-but-doesnt/72916318#72916318)
//auto const delimit_numeric_digits = []<typename T>(auto&& char_range, char const* name = "numeric digits" ) 
static auto const delimit_numeric_digits = [](auto&& char_range, char const* name = "numeric digits" ) 
{
    using T = double;
    auto constexpr max = std::numeric_limits<T>::max_digits10 * 2 - 1;
    decltype(max) constexpr min = 0;
    auto const chars = x3::char_(char_range);
    // clang-format off
    return x3::rule<struct _, std::string>{ name } = // x3::lexeme from outer rule
           x3::raw[chars >> x3::repeat(min, max)[('_' >> +chars | chars)]]
        >> !(chars | '_')
    ;
    // clang-format on
};
#else
static auto const delimit_numeric_digits = [](auto&& char_range, char const* name = "numeric digits" ) {
    auto const chars = x3::char_(char_range);
    // clang-format off
    return x3::rule<struct _, std::string>{ name } =
        x3::raw[chars >> *('_' >> +chars | chars)];
    // clang-format on
};
#endif

static auto const bin_digits = delimit_numeric_digits("01", "binary digits");
static auto const oct_digits = delimit_numeric_digits("0-7", "octal digits");
static auto const dec_digits = delimit_numeric_digits("0-9", "decimal digits");
static auto const hex_digits = delimit_numeric_digits("0-9a-fA-F", "hexadecimal digits");

///
/// create charsets by any given base in range [2...36]
/// concept [godbolt.org](https://godbolt.org/z/WTbsbW449)
///
auto const based_charset = [](unsigned base) {
    using namespace std::string_view_literals;
    namespace views = ranges::views;

    static auto constexpr digits = "0123456789"sv;
    static auto constexpr lower_letters = "abcdefghijklmnopqrstuvwxyz"sv;
    static auto constexpr upper_letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;

    // ensure up to 36 digits (10 digits + 26 letters)
    assert((2 <= base && base <= 36) && "Base must be in range [2, 36]");

    auto const dig_idx = base < digits.size() ? base : digits.size();
    auto const chr_idx = base > digits.size() ? base - dig_idx : 0;

    auto const char_list = {                                // --
        std::string_view(digits.data(), dig_idx),           // --
        std::string_view(lower_letters.data(), chr_idx),    // --
        std::string_view(upper_letters.data(), chr_idx)     // -
    };

    return ranges::to<std::string>(char_list | views::join);
};


#if 0 // unused
namespace detail {
//
// create charsets by given base at compile time (using constexpr)
// concept [godbolt](https://godbolt.org/z/6GPo36aeK)
// alternative simplified [godbolt](https://godbolt.org/z/3orETE9a4)
//
// original idea, @see [How to concatenate static strings at compile time?](
// https://stackoverflow.com/questions/38955940/how-to-concatenate-static-strings-at-compile-time)
//
template <std::size_t n>
struct String {
    std::array<char, n> p{};

    constexpr String() = default;

    constexpr String(char const (&pp)[n]) noexcept { ranges::copy(pp, std::begin(p)); }
    constexpr String(char const* pp) noexcept { ranges::copy_n(pp, n, std::begin(p)); }
    constexpr operator std::string_view() const noexcept { return { std::data(p), std::size(p) }; }
};

template <size_t n_lhs, size_t n_rhs>
constexpr String<n_lhs + n_rhs> operator+(String<n_lhs> lhs, String<n_rhs> rhs)
{
    String<n_lhs + n_rhs> ret(std::data(lhs.p));
    ranges::copy_n(std::begin(rhs.p), n_rhs, std::begin(ret.p) + n_lhs);
    return ret;
}

template <String... Strs>
struct join {
    // Join all strings into a single std::array of chars in static storage
    static auto constexpr arr = (Strs + ...);
    // View as a std::string_view
    static constexpr std::string_view value{ arr };
};

// Helper to get the value out
template <String... Strs>
static auto constexpr join_v = join<Strs...>::value;

template <unsigned Base>
requires BasicBaseRange<Base>  // --
    std::string_view constexpr based_charset_gen()
{
    using namespace std::string_view_literals;

    auto constexpr digits = "0123456789"sv;
    auto constexpr lower_case = "abcdefghijklmnopqrstuvwxyz"sv;
    auto constexpr upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
    static_assert(lower_case.size() == upper_case.size(), "");

    auto constexpr MAX_BASE = digits.size() + lower_case.size();

    static_assert(lower_case.size() == upper_case.size(), "");
    static_assert(2 <= Base && Base <= MAX_BASE, "Base must be in range [2, 36]");

    auto constexpr dig_idx = Base < digits.size() ? Base : digits.size();
    auto constexpr chr_idx = Base > digits.size() ? Base - dig_idx : 0;

    using namespace detail;

    auto constexpr dig = String<dig_idx>(digits.data());
    auto constexpr low = String<chr_idx>(lower_case.data());
    auto constexpr upr = String<chr_idx>(upper_case.data());

    auto constexpr chrset = join_v<dig, low, upr>;

    return chrset;
}

}  // namespace detail
#endif // unused

}  // namespace char_parser
}  // namespace parser
