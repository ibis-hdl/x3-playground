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

template <class... T>
static constexpr bool always_false = false;

template<typename T, template<typename> typename Derived>
struct promote_base {
    using base_type = T;
};

template<typename T>
struct promote : promote_base<T, promote> {
    static_assert(always_false<T>, "numeric type not supported");
};

template<>
struct promote<std::uint32_t> {
    using type = std::uint64_t;
};

template<>
struct promote<std::uint64_t> {
#if defined(_MSVC_LANG)
    // MSVC17 / VS2022 doesn't seems to have __uint128_t, hence we use
    // boost.multiprecision for simplicity; also see feature request 
    // [Support for 128-bit integer type](
    // https://developercommunity.visualstudio.com/t/support-for-128-bit-integer-type/879048)
    using type = ::boost::multiprecision::uint128_t;
#else
    using type = __uint128_t ;
#endif
    static_assert(sizeof(type) >= 16, "type with wrong promoted size");
};

template<>
struct promote<float> {
    using type = double;
};

template<>
struct promote<double> {
    using type = long double;
};


template<typename T>
struct safe_mul {

    T operator()(T lhs, T rhs, std::error_code& ec) const {
        using promote_type = typename promote<T>::type;
        promote_type const result = static_cast<promote_type>(lhs) * rhs;
        if(result > std::numeric_limits<T>::max()) {
            ec = std::make_error_code(std::errc::value_too_large);
        }
        return result;
    }
};

}  // namespace detail

// concept see [coliru](https://coliru.stacked-crooked.com/a/1fa02afbe79aec23)
// FixMe: [FP exceptions](https://en.cppreference.com/w/cpp/numeric/fenv/FE_exceptions)
template<typename T>
static detail::safe_mul<T> const mul = {};

}
