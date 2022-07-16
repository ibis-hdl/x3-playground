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

#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>

#include <charconv>
#include <system_error>

#include <string>
#include <string_view>
#include <cmath>
#include <numeric>  // accumulate

#include <iostream>
#include <iomanip>

namespace convert {

namespace detail {

auto const underline_predicate = [](char chr) { return chr != '_'; };

// prune the literal and copy result to string
static inline std::string remove_underline(auto literal)
{
    namespace views = ranges::views;

    return ranges::to<std::string>(literal | views::filter(underline_predicate));
}

// char-to-decimal for character range
std::uint32_t chr2dec(char chr);

template <IntergralType TargetT>
static inline auto as_integral_integer(unsigned base, std::string_view literal, std::error_code& ec)
{
    // got wrong results if the exponent is empty
    assert(!literal.empty() && "Attempt to to convert an empty literal");

    auto const clean_literal = convert::detail::remove_underline(literal);
    return from_chars<TargetT>(base, clean_literal, ec);
}

}  // namespace detail


template <UnsignedIntergralType IntT>
struct convert_integer {

    std::optional<IntT> operator()(ast::integer_type const& integer, std::error_code& ec) const
    {
        //std::cout << "convert_integer '" << integer << "'\n";

        auto const int_result = detail::as_integral_integer<IntT>(integer.base, integer.integer, ec);

        if (ec) {
            // error from std::from_chars()
            return {};
        }

        if (integer.exponent.empty()) {
            // nothings more to do
            return int_result;
        }

        auto const exp_scale = integer_exponent(integer.base, integer.exponent, ec);

        if (ec) {
            // error from std::from_chars() or exponent overflow
            return {};
        }

        auto const result = ::util::mul<IntT>(int_result, exp_scale, ec);

        if (ec) {
            // numeric range overflow
            return {};
        }

        return result;
    }

private:
    // exponent is signed
    IntT integer_exponent(unsigned base, std::string_view exponent_literal, std::error_code& ec) const
    {
        // decimal base for the exponent representation
        auto const exp_index = detail::as_integral_integer<IntT>(10U, exponent_literal, ec);

        if (ec) {
            // error from std::from_chars()
            return {}; // FixMe: throw exception
        }

        auto const exp_scale = detail::power<IntT>(base, exp_index, ec);

        if (ec) {
            // overflow
            return {}; // FixMe: throw exception
        }

        return exp_scale;
    }
};

template <typename TargetT>
static convert_integer<TargetT> const integer = {};


template <RealType RealT>
struct convert_real {
    // concept https://coliru.stacked-crooked.com/a/39b9d958b47f246b
    std::optional<RealT> operator()(ast::real_type const& real, std::error_code& ec) const
    {
        //std::cout << "convert_real '" << real << "'\n";

        // This intermediate type is required for converting parts of the real literal to
        // integer counterparts.
        // FixMe: make the concrete type depend on same IntergralType used for integer part. The
        // intent is to use the full sized table of this type; e.g. we ignored values if this type
        // would be of 64-bit.
        using promote_integer_type = std::uint32_t; // XXX

        if(real.base == 10U) {
            // std::from_chars() directly supports base 10 floating-point/real types
            auto const has_exponent = real.exponent.empty() == false;
            auto const real_string = as_real_string(real, has_exponent);
            auto const real_result = detail::from_chars<RealT>(real.base, real_string, ec);

            if (ec) {
                return {};
            }

            return real_result;
        }

        if(real.base == 16U) {
            // std::from_chars() directly supports base 16 floating-point/real types; but the
            // exponent support is of base 2, so use it without and multiply with the exponent
            // of base 16.
            // concept [coliru](https://coliru.stacked-crooked.com/a/596e7723bb7dc531)
            auto const with_exponent = false;
            auto const real_string = as_real_string(real, with_exponent);
            auto const real_result = detail::from_chars<RealT>(real.base, real_string, ec);

            if (ec) {
                // error from std::from_chars() or exponent overflow
                return {};
            }

            if (real.exponent.empty()) {
                // nothings more to do
                return real_result;
            }

            auto const exp_scale = real_exponent<promote_integer_type>(real.base, real.exponent, ec);

            if (ec) {
                // error from std::from_chars() or exponent overflow
                return {};
            }

            auto const result = ::util::mul<RealT>(real_result, exp_scale, ec);

            if (ec) {
                // numeric range overflow
                return {};
            }

            return result;
        }

        auto const int_result = detail::as_integral_integer<promote_integer_type>(real.base, real.integer, ec);

        if (ec) {
            // error from std::from_chars() or exponent overflow
            return {};
        }

        auto const frac_result = real_fractional(real.base, real.fractional);

        auto const real_result = ::util::add<RealT>(int_result, frac_result, ec);
        if (ec) {
            // overflow
            return {};
        }

        if(real.exponent.empty()) {
            // nothings more to do
            return real_result;
        }

        auto const exp_scale = real_exponent<promote_integer_type>(real.base, real.exponent, ec);

        if (ec) {
            // error from std::from_chars() or exponent overflow
            return {};
        }

        auto const result = ::util::mul<RealT>(real_result, exp_scale, ec);

        if (ec) {
            // numeric range overflow
            return {};
        }

        return result;
    }

private:
    // Join all parsed elements and prune delimiter '_' to prepare for call of
    // `from_chars()`. The result is a C++ standard conformance float string.
    // Concept: @see [godbolt.org](https://godbolt.org/z/KroM7oxc5)
    // @note To return a string prevents stack-use-after-scope, see
    // ([godbolt](https://godbolt.org/z/vr39h7Y8d)) but not relevant since std::string is returned.
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

    // TODO: replace naive implementation of fractional part calculation working
    RealT real_fractional(unsigned base, std::string_view fractional_literal) const
    {
        auto const fractional_string = detail::remove_underline(fractional_literal);

        RealT pow = static_cast<RealT>(base);

        auto const result = std::accumulate(std::begin(fractional_string), std::end(fractional_string), RealT{0},
            [&pow, base](RealT acc, char chr) {
                RealT dig = detail::chr2dec(chr);
                dig /= pow;
                pow *= static_cast<RealT>(base);
                return acc + dig;
            });

        return result;
    }

    template<IntergralType IntT>
    RealT real_exponent(unsigned base, std::string_view exponent_literal, std::error_code& ec) const
    {
        // exponent is signed
        using signed_type = std::make_signed_t<IntT>;

        // decimal base for the exponent representation
        auto const exp_index = detail::as_integral_integer<signed_type>(10U, exponent_literal, ec);

        if (ec) {
            // error from std::from_chars()
            return {}; // FixMe: throw exception
        }

        auto const exp_scale = detail::power<RealT>(base, exp_index, ec);

        if (ec) {
            // overflow
            return {}; // FixMe: throw exception
        }

        return exp_scale;
    }
};

template <typename TargetT>
static convert_real<TargetT> const real = {};

template <UnsignedIntergralType TargetT>
struct convert_bit_string_literal {

    std::optional<TargetT> operator()(ast::bit_string_literal const& literal, std::error_code& ec) const
    {
        auto const digit_string = detail::remove_underline(literal.literal);
        auto const binary_number = detail::from_chars<TargetT>(literal.base, digit_string, ec);

        if (ec) {
            return {};
        }

        return binary_number;
    }
};

template <typename TargetT>
static convert_bit_string_literal<TargetT> const bit_string_literal = {};

}  // namespace convert
