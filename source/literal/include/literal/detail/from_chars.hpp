//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/detail/constraint_types.hpp>
#include <literal/convert_error.hpp>

#include <boost/leaf/exception.hpp>
#include <boost/leaf/common.hpp>

#include <charconv>
#include <string>
#include <string_view>
#include <system_error>

#include <iostream>

namespace convert {

namespace detail {

template <typename T>
struct std_from_chars {
    static auto call(const char* const first, const char* const last, T& value, unsigned base)
    {
        static_assert(nostd::always_false<T>, "T must be of unsigned integral or float type");
    }
};

template <IntergralType IntT>
struct std_from_chars<IntT> {
    static auto call(const char* const first, const char* const last, IntT& value, unsigned base)
    {
        return std::from_chars(first, last, value, base);
    }
};

template <RealType RealT>
struct std_from_chars<RealT> {
    static auto call(const char* const first, const char* const last, RealT& value, unsigned base)
    {
        switch (base) {
            case 10:
                return std::from_chars(first, last, value, std::chars_format::general);
            case 16:
                // This floating-point literal must not have an exponent as in C++ the exponent
                // is of base 2, see [Floating-point literal](
                //  https://en.cppreference.com/w/cpp/language/floating_literal)
                return std::from_chars(first, last, value, std::chars_format::hex);
            default:
                // Support of other bases for floating-point literal as `std::from_chars()`
                // does is misleading.
                assert(
                    false &&
                    "Only Base 10 and 16 (without exponent) is supported for floating point types");
        }
        assert(false && "Wrong code path");
        // never goes here, but makes compiler quiet
        return std::from_chars_result{ first, std::errc::invalid_argument };
    }
};

template <typename TargetT>
class from_chars_api {
public:
    // low level API, call `std::from_chars()`, literal must be pruned from delimiter '_'
    TargetT operator()(unsigned base, std::string_view literal) const
    {
        literal = remove_positive_sign(literal);

        char const* const end = literal.data() + literal.size();

        TargetT result;
        auto const [ptr, errc] = std_from_chars<TargetT>::call(literal.data(), end, result, base);
        auto const ec = get_error_code(ptr, end, errc);

        if (ec) {
            throw leaf::exception(ec, leaf::e_api_function{ "from_chars" },
                                  leaf::e_position_iterator{ ptr });
        }

        return result;
    }

private:
    // removes a positive sign in front of the literal due to requirements of used underlying
    // [std::from_chars](https://en.cppreference.com/w/cpp/utility/from_chars):
    // "the plus sign is not recognized outside of the exponent" - for VHDL's integer exponent
    // it is allowed, and hence a valid rule for outer parser!
    static std::string_view remove_positive_sign(std::string_view literal)
    {
        if (literal.size() > 0 && literal[0] == '+') {
            literal.remove_prefix(1);
        }
        return literal;
    }

    // checks on `from_chars()` errc and on full match by comparing the iterators.
    static std::error_code get_error_code(char const* const ptr, char const* const end,
                                          std::errc errc)
    {
        if (ptr != end) {
            // something of input is wrong, outer parser fails
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (errc != std::errc{}) {
            // maybe errc::result_out_of_range, errc::invalid_argument ...
            return std::make_error_code(errc);
        }
        // all went fine
        return std::error_code{};
    }
};

template <typename TargetT>
static from_chars_api<TargetT> const from_chars = {};

}  // namespace detail

}  // namespace convert