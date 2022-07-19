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
/// assert(std::string_view{ "123456789" }.size() <= (digits_traits_v<std::uint32_t, 2>));
/// @endcode
///
template <typename T, unsigned Radix>
struct digits_traits : boost::spirit::x3::detail::digits_traits<T, Radix> {
};

template <typename T, unsigned Radix>
static std::size_t constexpr digits_traits_v = digits_traits<T, Radix>::value;

}  // namespace detail
}  // namespace convert

#if 0
// alternative constexpr implementation, using log and (for non-constexpr MSVC)
// sophisticated algorithm
template<typename T, unsigned Base>
auto constexpr digits_base = []() {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>,
                  "T must be of unsigned integral type");
    static_assert( 2 <= Base && Base <= 36, "Base must be in range [2, 36]");
#if 0  // HAVE_CONSTEXPR_LOG10
    auto constexpr logx = [](double base, double x) {
        return std::log10(x) / std::log10(base);
    };
    return std::floor(std::numeric_limits<T>::digits / logx(2, Base));
#else  // MSVC
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

    static_assert(digits_base<T,  2>() == convert::detail::digits_traits_v<T,  2>);
    // ...
    static_assert(digits_base<T, 36>() == convert::detail::digits_traits_v<T, 36>);
}
#endif
