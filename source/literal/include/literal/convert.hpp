//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>
#include <literal/detail/safe_math.hpp>
#include <literal/detail/power.hpp>
#include <literal/detail/from_chars.hpp>
#include <literal/detail/constraint_types.hpp>

#include <boost/leaf.hpp>
#include <literal/detail/leaf_errors.hpp>

#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>

#include <fmt/format.h>

#include <charconv>
#include <system_error>

#include <string>
#include <string_view>
#include <cmath>
#include <numeric>  // accumulate

#include <cfenv>

#include <iostream>
#include <iomanip>

namespace convert {

namespace leaf = boost::leaf;

namespace detail {

auto const underline_predicate = [](char chr) { return chr != '_'; };

/// 
/// Prune the literal from underline '_' and copy result to string
/// 
/// @param literal 
/// @return std::string 
///
static inline std::string remove_underline(auto literal)
{
    namespace views = ranges::views;

    return ranges::to<std::string>(literal | views::filter(underline_predicate));
}

/// 
/// char-to-decimal for character range with lookup O(1)
/// 
/// @param chr 
/// @return std::uint32_t 
///
/// maps '0-9', 'A-Z' and 'a-z' to their corresponding numeric value and maps all 
/// other characters to 0x7F (7-Bit ASCII 127d 'delete').
///
std::uint32_t chr2dec(char chr);

template <IntergralType TargetT>
static inline auto as_integral_integer(unsigned base, std::string_view literal)
{
    // FIXME in the past, got wrong results if e.g. the exponent literal was empty
    assert(!literal.empty() && "Attempt to to convert an empty literal");

    auto const clean_literal = convert::detail::remove_underline(literal);
    // LEAF
    return from_chars<TargetT>(base, clean_literal);
}

#if !defined(__APPLE__)
// Check that floating-point exceptions are supported and floating-point operations
// use the variable errno to report errors.
// @see [MATH_ERRNO, MATH_ERREXCEPT, math_errhandling](
//  https://en.cppreference.com/w/cpp/numeric/math/math_errhandling)
// @note This check is disabled on macOS since `math_errhandling` expands to non-constexpr
// function `__math_errhandling` and therefore cannot be used in a constant expression.
static bool constexpr has_iec60559_math = []() {
    return static_cast<bool>(math_errhandling & MATH_ERREXCEPT)  //
           && static_cast<bool>(math_errhandling & MATH_ERRNO);
}();

static_assert(has_iec60559_math,
              "Target must support floating-point exceptions and errno to report errors");
#endif // !__APPLE__

}  // namespace detail

template <UnsignedIntergralType IntT>
struct convert_integer {
    IntT operator()(ast::integer_type const& integer) const
    {
        // std::cout << "convert_integer '" << integer << "'\n";

        // LEAF from std::from_chars()
        auto const int_result = detail::as_integral_integer<IntT>(integer.base, integer.integer);

        if (integer.exponent.empty()) {
            // nothings more to do
            return int_result;
        }

        // LEAFfrom std::from_chars() or exponent overflow
        auto const exp_scale = integer_exponent(integer.base, integer.exponent);

        // LEAF numeric range overflow
        auto const result = ::util::mul<IntT>(int_result, exp_scale);

        return result;
    }

private:
    // integer exponent is unsigned
    IntT integer_exponent(unsigned base, std::string_view exponent_literal) const
    {
        // base for the exponent representation is always decimal
        static auto constexpr base10 = 10U;

        // LEAF- from_chars() may fail
        auto const exp_index = detail::as_integral_integer<IntT>(base10, exponent_literal);

        // LEAF
        auto const exp_scale = detail::power<IntT>(base, exp_index);

        return exp_scale;
    }
};

template <typename TargetT>
static convert_integer<TargetT> const integer = {};

template <RealType RealT>
struct convert_real {
    // concept https://coliru.stacked-crooked.com/a/39b9d958b47f246b
    RealT operator()(ast::real_type const& real) const
    {
        // std::cout << "convert_real '" << real << "'\n";

        // This intermediate type is required for converting parts of the real literal to
        // their integer counterparts.
        // -------------------------------------------------------------------------------
        // FixMe: make the concrete type depend on same IntergralType used for integer part. The
        // intent is to use the full sized table of this type; e.g. by use of 32-bit we wouldn't use
        // all possible lookup values as this type would be of 64-bit.
        // -------------------------------------------------------------------------------
        using promote_integer_type = std::uint32_t;

        if (real.base == 10U) {
            // std::from_chars() directly supports base 10 floating-point/real types
            auto const with_exponent = !real.exponent.empty();
            auto const real_string = as_real_string(real, with_exponent);
            // LEAF
            auto const real_result = detail::from_chars<RealT>(real.base, real_string);

            return real_result;
        }

        if (real.base == 16U) {
            // std::from_chars() directly supports base 16 floating-point/real types; but the
            // exponent support is of base 2, so use it without and multiply with the exponent
            // of base 16 later on (if necessary).
            // concept [coliru](https://coliru.stacked-crooked.com/a/596e7723bb7dc531)
            auto const with_exponent = false;
            auto const real_string = as_real_string(real, with_exponent);
            // LEAF
            auto const real_result = detail::from_chars<RealT>(real.base, real_string);

            if (real.exponent.empty()) {
                // nothings more to do
                return real_result;
            }

            // LEAF
            auto const exp_scale = real_exponent<promote_integer_type>(real.base, real.exponent);

            // LEAF
            auto const result = ::util::mul<RealT>(real_result, exp_scale);

            return result;
        }
        
        // other bases follow, which aren't directly supported by `from_chars()`

        // LEAF
        auto const int_result =
            detail::as_integral_integer<promote_integer_type>(real.base, real.integer);

        auto const frac_result = real_fractional(real.base, real.fractional);

        // LEAF
        auto const real_result = ::util::add<RealT>(int_result, frac_result);

        if (real.exponent.empty()) {
            // nothings more to do
            return real_result;
        }

        // LEAF
        auto const exp_scale = real_exponent<promote_integer_type>(real.base, real.exponent);

        // LEAF
        auto const result = ::util::mul<RealT>(real_result, exp_scale);

        return result;
    }

private:
    // Join all parsed elements and prune delimiter '_' to prepare for call of
    // `from_chars()`. The result is a C++ standard conformance floating point string.
    // Concept: @see [godbolt.org](https://godbolt.org/z/KroM7oxc5)
    // @note To return a string prevents stack-use-after-scope, see
    // ([godbolt](https://godbolt.org/z/vr39h7Y8d))
    std::string as_real_string(ast::real_type const& real, bool with_exponent) const
    {
        using namespace std::literals::string_view_literals;

        auto const remove_underline = [](auto const& literal_list) {
            namespace views = ranges::views;
            return ranges::to<std::string>(literal_list | views::join |
                                           views::filter(detail::underline_predicate));
        };

        auto const as_sv = [](std::string const& str) { return std::string_view(str); };

        if (with_exponent) {
            // clang-format off
            auto const literal_list = {
                as_sv(real.integer), "."sv, as_sv(real.fractional), "e"sv, as_sv(real.exponent)
            };
            // clang-format on

            return remove_underline(literal_list);
        }

        // clang-format off
        auto const literal_list = {
            as_sv(real.integer), "."sv, as_sv(real.fractional)
        };
        // clang-format on

        return remove_underline(literal_list);
    }

    // TODO: replace naive implementation of fractional part calculation
    // TODO: The summand can get small, consider to use [Kahan Summation](
    // https://stackoverflow.com/questions/10330002/sum-of-small-double-numbers-c)
    RealT real_fractional(unsigned base, std::string_view fractional_literal) const
    {
        LEAF_ERROR_TRACE;

        // paranoia, but a base of 0 would result into division by zero by the algorithm used
        assert(base > 0);

        auto const fractional_string = detail::remove_underline(fractional_literal);

        RealT pow = static_cast<RealT>(base);

        std::feclearexcept(FE_ALL_EXCEPT);

        auto const result =
            std::accumulate(std::begin(fractional_string), std::end(fractional_string), RealT{ 0 },
                            [&pow, base](RealT acc, char chr) {
                                auto dig = static_cast<RealT>(detail::chr2dec(chr));
                                dig /= pow;
                                pow *= static_cast<RealT>(base);
                                return acc + dig;
                            });

        int const fp_exception_raised = std::fetestexcept(FE_ALL_EXCEPT & ~FE_INEXACT);

        if (fp_exception_raised) {
            std::feclearexcept(FE_ALL_EXCEPT);
            auto const ec = std::error_code(errno, std::generic_category());
            throw leaf::exception(ec, leaf::e_api_function{ "safe_add<RealT>" },
                                  leaf::e_fp_exception{ fp_exception_raised });
        }

        return result;
    }

    template <IntergralType IntT>
    RealT real_exponent(unsigned base, std::string_view exponent_literal) const
    {
        // exponent is signed
        using signed_type = std::make_signed_t<IntT>;

        // base for the exponent representation is always decimal
        static auto constexpr base10 = 10U;

        // LEAF from std::from_chars()
        auto const exp_index = detail::as_integral_integer<signed_type>(base10, exponent_literal);

        // LEAF
        auto const exp_scale = detail::power<RealT>(base, exp_index);

        return exp_scale;
    }
};

template <typename TargetT>
static convert_real<TargetT> const real = {};

template <UnsignedIntergralType TargetT>
struct convert_bit_string_literal {
    TargetT operator()(ast::bit_string_literal const& literal) const
    {
        auto const digit_string = detail::remove_underline(literal.literal);
        // LEAF
        auto const binary_number = detail::from_chars<TargetT>(literal.base, digit_string);

        return binary_number;
    }
};

template <typename TargetT>
static convert_bit_string_literal<TargetT> const bit_string_literal = {};

}  // namespace convert
