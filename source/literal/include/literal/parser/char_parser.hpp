//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>

#include <string_view>
#include <string>
#include <cassert>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace char_parser {

// FixMe: [Misunderstanding repeat directive - it should fail, but doesn't](
// https://stackoverflow.com/questions/72915071/misunderstanding-repeat-directive-it-should-fail-but-doesnt/72916318#72916318)
auto const delimit_numeric_digits = [](auto&& char_range, char const* name) {
    auto cs = x3::char_(char_range);
    // clang-format off
    return x3::rule<struct _, std::string>{ "numeric digits" } =
        x3::raw[cs >> *('_' >> +cs | cs)];
    // clang-format on
};

auto const bin_integer = delimit_numeric_digits("01", "binary digits");
auto const oct_integer = delimit_numeric_digits("0-7", "octal digits");
auto const dec_integer = delimit_numeric_digits("0-9", "decimal digits");
auto const hex_integer = delimit_numeric_digits("0-9a-fA-F", "hexadecimal digits");

///
/// create charsets by given base
/// concept [godbolt.org](https://godbolt.org/z/T5YEYhE54)
///
auto const based_charset = [](unsigned base)
{
    using namespace std::string_view_literals;
    namespace views = ranges::views;

    static auto constexpr digits = "0123456789"sv;
    static auto constexpr lower_case = "abcdefghijklmnopqrstuvwxyz"sv;
    static auto constexpr upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
    auto constexpr MAX_BASE = digits.size() + lower_case.size();

    static_assert(lower_case.size() == upper_case.size(), "");
    assert((base > 1 && base <= MAX_BASE) && "Base must be in range [2,36]");

    auto const dig_idx = base < digits.size() ? base : digits.size();
    auto const chr_idx = base > digits.size() ? base - dig_idx : 0;

    // clang-format off
    auto const char_list = {
        std::string_view(digits.data(), dig_idx),
        std::string_view(lower_case.data(), chr_idx),
        std::string_view(upper_case.data(), chr_idx)
    };
    // clang-format on
    return ranges::to<std::string>(char_list | views::join);
};

auto const based_integer = [](unsigned base, char const* name = "based integer") {
    return delimit_numeric_digits(based_charset(base), name);
};

namespace detail {
//
// create charsets by given base at compile time (using constexpr)
// concept [godbolt](https://godbolt.org/z/6GPo36aeK)
// alternative simplified [godbolt](https://godbolt.org/z/3orETE9a4)
//
// original idea, @see [How to concatenate static strings at compile time?](
// https://stackoverflow.com/questions/38955940/how-to-concatenate-static-strings-at-compile-time)
//
template<std::size_t n>
struct String
{
    std::array<char, n> p{};

    constexpr String() = default;

    constexpr String(char const(&pp)[n]) noexcept {
        std::ranges::copy(pp, std::begin(p));
    };
    constexpr String(char const* pp) noexcept {
        std::ranges::copy_n(pp, n, std::begin(p));
    };
    constexpr operator std::string_view() const noexcept {
        return {std::data(p), std::size(p)};
    }
};

template<size_t n_lhs, size_t n_rhs>
constexpr String<n_lhs + n_rhs> operator+(String<n_lhs> lhs, String<n_rhs> rhs)
{
    String<n_lhs + n_rhs> ret(std::data(lhs.p));
    std::ranges::copy_n(std::begin(rhs.p), n_rhs, std::begin(ret.p) + n_lhs);
    return ret;
}

template <String... Strs>
struct join
{
    // Join all strings into a single std::array of chars in static storage
    static constexpr auto arr = (Strs + ...);
    // View as a std::string_view
    static constexpr std::string_view value {arr};
};

// Helper to get the value out
template <String... Strs>
static constexpr auto join_v = join<Strs...>::value;

} // namespace detail

template<unsigned Base>
std::string_view constexpr based_charset_gen()
{
    using namespace std::string_view_literals;

    auto constexpr digits = "0123456789"sv;
    auto constexpr lower_case = "abcdefghijklmnopqrstuvwxyz"sv;
    auto constexpr upper_case = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
    static_assert(lower_case.size() == upper_case.size(), "");

    auto constexpr MAX_BASE = digits.size() + lower_case.size();

    static_assert((Base > 1) && (Base <= MAX_BASE), "Base must be in range [2,36]");

    auto constexpr dig_idx = Base < digits.size() ? Base : digits.size();
    auto constexpr chr_idx = Base > digits.size() ? Base - dig_idx : 0;

    using namespace detail;

    auto constexpr d = String<dig_idx>(digits.data());
    auto constexpr l = String<chr_idx>(lower_case.data());
    auto constexpr u = String<chr_idx>(upper_case.data());

    constexpr auto chrset = join_v<d, l, u>;

    return chrset;
}

//static auto const dec_charset = based_charset_gen<10>();

}  // namespace char_parser
}  // namespace parser
