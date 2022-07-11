// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/spirit/home/x3/support/numeric_utils/detail/extract_int.hpp>

namespace convert {

namespace detail {

///
/// The maximum radix digits that can be represented without overflow
///
/// @tparam T Underlying type
/// @tparam Radix Radix of interest represented in decimal system
///
/// @note Simple derived from Spirit X3 for convenience
///
/// Usage, e.g.:
/// @code{.cpp}
/// assert(std::string_view{ "123456789" }.size() <= (digits_traits<std::uint32_t, 2>::value));
/// @endcode
///
template <typename T, unsigned Radix>
struct digits_traits : boost::spirit::x3::detail::digits_traits<T, Radix> {
};

}  // namespace detail
}  // namespace convert

#if 0
// alternative constexpr implementation, using log and (for non-constexpr MSVC)
// sophisticated algorithm
template<typename T, unsigned Base>
auto constexpr digits_base = []() {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>,
                  "T must be of unsigned integral type");
    static_assert( Base > 1 && Base < 37, "Base must be in range [2, 36]");
#if 0 // HAVE_CONSTEXPR_LOG10
    auto constexpr logx = [](double base, double x) {
        return std::log10(x) / std::log10(base);
    };
    return std::floor(std::numeric_limits<T>::digits / logx(2, Base));
#else // MSVC
    // see concept [godbolt.org](https://godbolt.org/z/fzzq6hMnv)
    double param = std::numeric_limits<T>::max();
    std::size_t result = 0;
    for(; param > 1; param /= Base) ++result;
    // also adjustment to account for the leading digit
    return static_cast<double>(result - (unsigned(param * Base) != Base - 1));
#endif
};

void show_digits()
{
    using T = std::uint32_t;
    unsigned constexpr base = 4;
    unsigned constexpr digits_n = digits_base<T, base>();

    std::cout << fmt::format("numeric_limits(digits10: {}, digits2: {}); digits{}: {}\n",
        std::numeric_limits<T>::digits10,
        std::numeric_limits<T>::digits,
        base,
        digits_n
        );

    static_assert(digits_base<T,  2>() == convert::detail::digits_traits<T,  2>::value);
    static_assert(digits_base<T,  3>() == convert::detail::digits_traits<T,  3>::value);
    static_assert(digits_base<T,  4>() == convert::detail::digits_traits<T,  4>::value);
    static_assert(digits_base<T,  5>() == convert::detail::digits_traits<T,  5>::value);
    static_assert(digits_base<T,  6>() == convert::detail::digits_traits<T,  6>::value);
    static_assert(digits_base<T,  7>() == convert::detail::digits_traits<T,  7>::value);
    static_assert(digits_base<T,  8>() == convert::detail::digits_traits<T,  8>::value);
    static_assert(digits_base<T,  9>() == convert::detail::digits_traits<T,  9>::value);
    static_assert(digits_base<T, 10>() == convert::detail::digits_traits<T, 10>::value);
    static_assert(digits_base<T, 11>() == convert::detail::digits_traits<T, 11>::value);
    static_assert(digits_base<T, 12>() == convert::detail::digits_traits<T, 12>::value);
    static_assert(digits_base<T, 13>() == convert::detail::digits_traits<T, 13>::value);
    static_assert(digits_base<T, 14>() == convert::detail::digits_traits<T, 14>::value);
    static_assert(digits_base<T, 15>() == convert::detail::digits_traits<T, 15>::value);
    static_assert(digits_base<T, 16>() == convert::detail::digits_traits<T, 16>::value);
    static_assert(digits_base<T, 17>() == convert::detail::digits_traits<T, 17>::value);
    static_assert(digits_base<T, 18>() == convert::detail::digits_traits<T, 18>::value);
    static_assert(digits_base<T, 19>() == convert::detail::digits_traits<T, 19>::value);
    static_assert(digits_base<T, 20>() == convert::detail::digits_traits<T, 20>::value);
    static_assert(digits_base<T, 21>() == convert::detail::digits_traits<T, 21>::value);
    static_assert(digits_base<T, 22>() == convert::detail::digits_traits<T, 22>::value);
    static_assert(digits_base<T, 23>() == convert::detail::digits_traits<T, 23>::value);
    static_assert(digits_base<T, 24>() == convert::detail::digits_traits<T, 24>::value);
    static_assert(digits_base<T, 25>() == convert::detail::digits_traits<T, 25>::value);
    static_assert(digits_base<T, 26>() == convert::detail::digits_traits<T, 26>::value);
    static_assert(digits_base<T, 27>() == convert::detail::digits_traits<T, 27>::value);
    static_assert(digits_base<T, 28>() == convert::detail::digits_traits<T, 28>::value);
    static_assert(digits_base<T, 29>() == convert::detail::digits_traits<T, 29>::value);
    static_assert(digits_base<T, 30>() == convert::detail::digits_traits<T, 30>::value);
    static_assert(digits_base<T, 31>() == convert::detail::digits_traits<T, 31>::value);
    static_assert(digits_base<T, 32>() == convert::detail::digits_traits<T, 32>::value);
    static_assert(digits_base<T, 33>() == convert::detail::digits_traits<T, 33>::value);
    static_assert(digits_base<T, 34>() == convert::detail::digits_traits<T, 34>::value);
    static_assert(digits_base<T, 35>() == convert::detail::digits_traits<T, 35>::value);
    static_assert(digits_base<T, 36>() == convert::detail::digits_traits<T, 36>::value);
}
#endif
