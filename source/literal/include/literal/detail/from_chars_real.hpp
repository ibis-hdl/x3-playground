//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#if !defined(CONVERT_FROM_CHARS_USE_STRTOD)
#error This is not intended to be used out of libc++
#endif

#include <literal/detail/constraint_types.hpp>

#include <fmt/format.h>

#include <new>      // bad_alloc
#include <utility>  // exchange
#include <cstdlib>  // strtof, strtod, strtold
#include <cmath>    // isinf
#include <fenv.h>   // fegetround, fesetround

namespace convert::detail {

template <typename T> struct std_from_chars;

///
/// At least until libc++ 16 there is no implementation from `from-chars` for real types.
/// This affects libc++ users as on macOS using Clang. 
/// This is a quick & dirty solution using `strtod` and a temporary 'fake' buffer to support
/// hex floats to address this issue.
///
template <RealType RealT>
struct std_from_chars<RealT> {
    static auto call(const char* const first, const char* const last, RealT& value, unsigned base) noexcept
    {
        switch (base) {
            case 10: {
                    std::errc ec = std::errc::invalid_argument;
                    std::size_t const n_diff = impl(first, value, ec);
                    return make_result(first, n_diff, ec);
                }

            case 16: {
                    std::errc ec = std::errc::invalid_argument;
                    // create a fake buffer for use with `strtod`and friends for hex floats
                    auto buf = fmt::memory_buffer();
                    auto [out, size] = fmt::format_to_n(std::back_inserter(buf), buf.capacity() - 1, "0x{}", std::string_view(first, last));
                    *out = '\0';
                    std::size_t n_diff = impl(buf.begin(), value, ec);
                    n_diff -= 2; // correct for the "0x"
                    return make_result(first, n_diff, ec);
                }

            default:
                return std::from_chars_result{ first, std::errc::not_supported };
        }
        // never goes here, but makes compiler quiet
        return std::from_chars_result{ first, std::errc::not_supported };
    }

private:
    static std::ptrdiff_t impl(const char* const first, RealT& value, std::errc& ec) noexcept
    {
        int const rounding = std::fegetround();
        if (rounding != FE_TONEAREST) {
            std::fesetround(FE_TONEAREST);
        }

        int const errno_bak = errno;
        errno = 0;
        char* endptr;   // no nullptr! see docs
        RealT temp_result;

        if constexpr (std::is_same_v<RealT, float>) {
            temp_result = std::strtof(first, &endptr);
        } else if constexpr (std::is_same_v<RealT, double>) {
            temp_result = std::strtod(first, &endptr);
        } else if constexpr (std::is_same_v<RealT, long double>) {
            temp_result = std::strtold(first, &endptr);
        }
        
        int const conv_errno = std::exchange(errno, errno_bak);
        ptrdiff_t const n_ptrdiff = endptr - first;

        if (conv_errno == ERANGE) [[unlikely]] {
            if (std::isinf(temp_result)) { // overflow
                ec = std::errc::result_out_of_range;
            } else { // underflow
                ec = std::errc::result_out_of_range;
            }
        } 
        else if (n_ptrdiff) {
            value = temp_result;
            ec = std::errc{};
        }

        return n_ptrdiff;
    }

    static std::from_chars_result make_result(const char* const start, ptrdiff_t n_diff, std::errc ec) noexcept
    {
        std::from_chars_result result = { start, ec };

        if (n_diff != 0) {
            result.ptr += n_diff;
        }

        return result;
    }
};

} // namespace convert::detail 
