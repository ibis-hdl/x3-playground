//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <range/v3/view/filter.hpp>
#include <range/v3/range/conversion.hpp>
#include <charconv>

#include <string>
#include <string_view>

#include <iostream>
#include <iomanip>

namespace convert {

namespace detail {

// prune the literal and copy result to string
static inline std::string prune(auto literal)
{
    namespace views = ranges::views;

    return ranges::to<std::string>(literal | views::filter([](char chr) { return chr != '_'; }));
}

template <typename TargetT>
static inline auto as_unsigned(std::string_view literal, unsigned base, std::error_code& ec)
{
    static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                  "TargetT must be of unsigned integral type");

    // std::from_chars() works with raw char pointers
    char const* const end = literal.data() + literal.size();

    TargetT result = static_cast<TargetT>(-1);
    auto const [ptr, errc] = std::from_chars(literal.data(), end, result, base);

#if !defined(__clang__)
    // [reference to local binding fails in enclosing function #48582](
    //  https://github.com/llvm/llvm-project/issues/48582)
    auto const get_error_code = [&]() {
        if (ptr != end) {
            // something of input is wrong, outer parser fails
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (errc != std::errc{}) {
            // maybe errc::result_out_of_range, errc::invalid_argument ...
            return std::make_error_code(errc);
        }
        return std::error_code{};
    };

    ec = get_error_code();
#else
#warning Using Clang work-around
    if (ptr != end) {
        // something of input is wrong, outer parser fails
        ec = std::make_error_code(std::errc::invalid_argument);
    }
    if (errc != std::errc{}) {
        // maybe errc::result_out_of_range, errc::invalid_argument ...
        ec = std::make_error_code(errc);
    }
    ec = std::error_code{};
#endif

    return result;
}

template <typename TargetT>
static inline auto as_real(std::string_view literal, unsigned base, std::error_code& ec)
{
    static_assert(std::is_floating_point_v<TargetT>, "TargetT must be of floating point type");

    assert(base == 10U && "only base 10 is supported");

    // std::from_chars() works with raw char pointers
    const char* const end = literal.data() + literal.size();

    TargetT result;  // = std::numeric_limits<TargetT>::quiet_NaN();
    auto const [ptr, errc] =
        std::from_chars(literal.data(), end, result, std::chars_format::general);

#if !defined(__clang__)
    // [reference to local binding fails in enclosing function #48582](
    //  https://github.com/llvm/llvm-project/issues/48582)
    auto const get_error_code = [&]() {
        if (ptr != end) {
            // something of input is wrong, outer parser fails
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (errc != std::errc{}) {
            // maybe errc::result_out_of_range, errc::invalid_argument ...
            return std::make_error_code(errc);
        }
        return std::error_code{};
    };

    ec = get_error_code();
#else
#warning Using Clang work-around
    if (ptr != end) {
        // something of input is wrong, outer parser fails
        ec = std::make_error_code(std::errc::invalid_argument);
    }
    if (errc != std::errc{}) {
        // maybe errc::result_out_of_range, errc::invalid_argument ...
        ec = std::make_error_code(errc);
    }
    ec = std::error_code{};
#endif

    return result;
}

// concept, see [coliru](https://coliru.stacked-crooked.com/a/ee869b77c78b496d)
template <typename T, unsigned Base>
class exp_scale {
    static_assert(std::is_integral_v<T>, "T must be integral type");

public:
    using value_type = T;

public:
    constexpr exp_scale()
    {
        assert(Base == 10U && "This table was developed only with Base 10 in mind!");

        T value = 1;
        for (std::size_t i = 0; i != SIZE; ++i) {
            data[i] = value;
            value *= Base;
        }
    }

    T operator[](unsigned idx) const
    {
        assert(idx < SIZE && "exponent index out of range");
        return data[idx];
    }

    unsigned max_index() const { return SIZE; }

private:
    static constexpr auto SIZE = std::numeric_limits<T>::digits10 + 1;
    std::array<value_type, SIZE> data;
};

template <typename T>
static constexpr exp_scale<T, 10U> exp10_scale = {};

}  // namespace detail
}  // namespace convert
