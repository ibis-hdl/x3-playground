//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <type_traits>
#include <limits>
#include <cstdint>
#include <system_error>

#if defined(_MSVC_LANG)
#include <boost/multiprecision/cpp_int.hpp>
#endif

namespace util {

namespace detail {

// promote type concept see [coliru](https://coliru.stacked-crooked.com/a/ca649fc42f0b13c9)
template <typename T>
struct promote {
    static_assert(sizeof(T) == 0, "numeric type not supported");
};

template <>
struct promote<std::uint32_t> {
    using type = std::uint64_t;
};

template <>
struct promote<std::uint64_t> {
#if defined(_MSVC_LANG)
    // MSVC17 / VS2022 doesn't seems to have __uint128_t, hence we use
    // boost.multiprecision for simplicity; also see feature request
    // [Support for 128-bit integer type](
    // https://developercommunity.visualstudio.com/t/support-for-128-bit-integer-type/879048)
    using type = ::boost::multiprecision::uint128_t;
#else
    using type = __uint128_t;
#endif
    static_assert(sizeof(type) >= 16, "type with wrong promoted size");
};

template <>
struct promote<float> {
    using type = double;
};

template <>
struct promote<double> {
    using type = long double;
};

// Select numeric implementation ((unsigned) integer and real)
// concept see [coliru](https://coliru.stacked-crooked.com/a/dd9e4543247597ad)
template <typename T, class Enable = void>
struct safe_mul {
    static_assert(sizeof(T) == 0, "unsupported numeric type");
};

template <typename IntT>
struct safe_mul<IntT, typename std::enable_if_t<                                // --
                          std::is_integral_v<IntT> && std::is_unsigned_v<IntT>  // --
                          && !std::is_same_v<IntT, bool>>> {
    IntT operator()(IntT lhs, IntT rhs, std::error_code& ec) const
    {
        using promote_type = typename promote<IntT>::type;
        promote_type const result = static_cast<promote_type>(lhs) * rhs;

        if (result > std::numeric_limits<IntT>::max()) {
            ec = std::make_error_code(std::errc::value_too_large);
        }
        return static_cast<IntT>(result);
    }
};

// FixMe: Support [FP exceptions](https://en.cppreference.com/w/cpp/numeric/fenv/FE_exceptions)
template <typename RealT>
struct safe_mul<RealT, typename std::enable_if_t<std::is_floating_point_v<RealT>>> {
    RealT operator()(RealT lhs, RealT rhs, std::error_code& ec) const
    {
        using promote_type = typename promote<RealT>::type;
        promote_type const result = static_cast<promote_type>(lhs) * rhs;

        if (result > std::numeric_limits<RealT>::max()) {
            ec = std::make_error_code(std::errc::value_too_large);
        }
        return static_cast<RealT>(result);
    }
};

}  // namespace detail

template <typename T>
static detail::safe_mul<T> const mul = {};

}  // namespace util
