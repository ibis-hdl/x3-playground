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

template <typename TargetT, unsigned Base>
class from_chars_api
{
public:
    TargetT operator()(std::string_view literal, std::error_code& ec) const {

        // @note [from_chars](https://en.cppreference.com/w/cpp/utility/from_chars):
        // "the plus sign is not recognized outside of the exponent" - for VHDL's integer exponent
        // this is allowed, and it's a valid rule for outer parser! Let's purge it.
        if(literal.size() > 0 && literal[0] == '+') {
            literal.remove_prefix(1);
        }

        char const* const end = literal.data() + literal.size();

        // FixMe: What happens here? https://coliru.stacked-crooked.com/a/2164be2b3363cbf5
        // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2593r0.html

        auto const from_char_ = [&](TargetT& result) {
            if constexpr (std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>) {
                return std::from_chars(literal.data(), end, result, Base);
            }
            else if constexpr (std::is_floating_point_v<TargetT>) {
                assert(Base == 10U && "Only Base = 10 is supported for floating point");
                return std::from_chars(literal.data(), end, result, std::chars_format::general);
            }
            else {
                // must be of type unsigned integer or float
                // FixMe: make this compile time error
                assert(false && "unsupported numeric target type");
                return 0;
            }
        };

        TargetT result;
        auto const [ptr, errc] = from_char_(result);

        ec = get_error_code(ptr, end, errc);
        return result;
    }

private:
    std::error_code get_error_code(char const* const ptr, char const* const end, std::errc errc) const {
        if (ptr != end) {
            // something of input is wrong, outer parser fails
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (errc != std::errc{}) {
            // maybe errc::result_out_of_range, errc::invalid_argument ...
            return std::make_error_code(errc);
        }
        return std::error_code{};
    }
};

template<typename TargetT, unsigned Base>
static const from_chars_api<TargetT, Base> from_chars = {};

template <typename TargetT, unsigned Base>
static inline auto as_unsigned(std::string_view literal, std::error_code& ec)
{
    static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                  "TargetT must be of unsigned integral type");

    return from_chars<TargetT, Base>(literal, ec);
}

template <typename TargetT, unsigned Base>
static inline auto as_real(std::string_view literal, std::error_code& ec)
{
    static_assert(std::is_floating_point_v<TargetT>, "TargetT must be of floating point type");
    static_assert(Base == 10U && "only base 10 for real type is supported");

    return from_chars<TargetT, Base>(literal, ec);
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
