//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/convert/detail/int_types.hpp>
#include <literal/convert/detail/constraint_types.hpp>

#include <boost/leaf/exception.hpp>
#include <boost/leaf/common.hpp>
#include <literal/convert/leaf_errors.hpp>

#include <type_traits>
#include <limits>
#include <cstdint>
#include <cfenv>
#include <system_error>
#include <iostream>

namespace util {

namespace leaf = boost::leaf;

namespace detail {

// promote type concept see [Coliru](https://coliru.stacked-crooked.com/a/ca649fc42f0b13c9)
template <typename T>
struct promote {
    static_assert(nostd::always_false<T>, "numeric type not supported");
};

template <>
struct promote<std::uint32_t> {
    using type = std::uint64_t;
};

template <>
struct promote<std::uint64_t> {
    using type = nostd::uint128_t;
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

template <typename T>
using promote_t = typename promote<T>::type;

// low overhead math, where [Safe Numerics](
//  https://www.boost.org/doc/libs/develop/libs/safe_numerics/doc/html/index.html)
// would be overkill
//
// FixMe: rewrote to generalize the implementations, e.g.
//   binary_operation<T, operation<T>> where operation<T> is mul, add, ...
// working on:https://godbolt.org/z/T7xeh7d6W
// SO: https://stackoverflow.com/questions/73066724/cant-check-for-overflow-of-double-float-operations-by-use-of-machines-epsilon
// Problem with overflow detection of double

// Select numeric implementation ((unsigned) integer and real)
// concept see [Coliru](https://coliru.stacked-crooked.com/a/dd9e4543247597ad)
template <typename T>
struct safe_mul {
    static_assert(nostd::always_false<T>, "unsupported numeric type");
};

template <typename T>
struct safe_add {
    static_assert(nostd::always_false<T>, "unsupported numeric type");
};

// To multiply type promotion is used; alternative for integer see
// [Catch and compute overflow during multiplication of two large integers](
//  https://stackoverflow.com/questions/1815367/catch-and-compute-overflow-during-multiplication-of-two-large-integers)
template <IntegralType IntT>
struct safe_mul<IntT> {
    IntT operator()(IntT lhs, IntT rhs) const
    {
        LEAF_ERROR_TRACE;

        auto const result = static_cast<promote_t<IntT>>(lhs) * rhs;

        if (result > std::numeric_limits<IntT>::max()) {
            auto const ec = std::make_error_code(std::errc::result_out_of_range);
            throw leaf::exception(ec, leaf::e_api_function{ "safe_mul<IntT>" });
        }

        return static_cast<IntT>(result);
    }
};

template <RealType RealT>
struct safe_mul<RealT> {
    RealT operator()(RealT lhs, RealT rhs) const
    {
        LEAF_ERROR_TRACE;

        std::feclearexcept(FE_ALL_EXCEPT);

        auto const result = lhs * rhs;

        int const fp_exception_raised = std::fetestexcept(FE_ALL_EXCEPT & ~FE_INEXACT);

        if (fp_exception_raised) {
            std::feclearexcept(FE_ALL_EXCEPT);
            auto const ec = std::error_code(errno, std::generic_category());
            throw leaf::exception(ec, leaf::e_api_function{ "safe_mul<RealT>" },
                                  leaf::e_fp_exception{ fp_exception_raised });
        }

        return result;
    }
};

template <RealType RealT>
struct safe_add<RealT> {
    RealT operator()(RealT lhs, RealT rhs) const
    {
        LEAF_ERROR_TRACE;

        std::feclearexcept(FE_ALL_EXCEPT);

        auto const result = lhs + rhs;

        int const fp_exception_raised = std::fetestexcept(FE_ALL_EXCEPT & ~FE_INEXACT);

        if (fp_exception_raised) {
            std::feclearexcept(FE_ALL_EXCEPT);
            auto const ec = std::error_code(errno, std::generic_category());
            throw leaf::exception(ec, leaf::e_api_function{ "safe_add<RealT>" },
                                  leaf::e_fp_exception{ fp_exception_raised });
        }

        return result;
    }
};

}  // namespace detail

template <typename T>
static detail::safe_mul<T> const mul = {};

template <typename T>
static detail::safe_add<T> const add = {};

}  // namespace util
