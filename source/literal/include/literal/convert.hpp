//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>
#include <literal/detail/safe_mul.hpp>
#include <literal/detail/digit_traits.hpp>

#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>
#include <charconv>

#include <string>
#include <string_view>

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

// ---

// FixMe https://godbolt.org/z/4xWaGs7dq (C++20 concept)

template<typename T, class Enable = void>
struct api
{
    static auto call(const char* const first, const char* const last, T& value, unsigned base)
    {
        static_assert(sizeof(T) && false, "T must be of unsigned integral or float type");
    }
};

template<typename IntT>
struct api<IntT, typename std::enable_if_t<                                   // --
                        std::is_integral_v<IntT> && std::is_unsigned_v<IntT>  // --
                        && !std::is_same_v<IntT, bool>>>
{
    static auto call(const char* const first, const char* const last, IntT& value, unsigned base)
    {
        return std::from_chars(first, last, value, base);
    }
};

template<typename RealT>
struct api<RealT, typename std::enable_if_t<std::is_floating_point_v<RealT>>> {
    static auto call(const char* const first, const char* const last, RealT& value, unsigned base)
    {
        assert(base == 10U && "Only Base = 10 is supported for floating point");

        // FixMe: other bases

        return std::from_chars(first, last, value, std::chars_format::general);
    }
};

// ---

template <typename TargetT>
class from_chars_api {
public:
    // low level API, call `std::from_chars()`, literal must be pruned from delimiter '_'
    TargetT operator()(std::string_view literal, unsigned base, std::error_code& ec) const
    {
        literal = remove_positive_sign(literal);

        char const* const end = literal.data() + literal.size();

        //std::cout << "from_chars_api: " << "b=" << base << "'" << literal << "'\n";
        TargetT result;
        auto const [ptr, errc] = api<TargetT>::call(literal.data(), end, result, base);
        ec = get_error_code(ptr, end, errc);
#if 0
        std::cout << "from_chars_api: " << "b=" << base << "'" << literal << "' (";
        if (ec) { std::cout << "fail)\n";  }
        else { std::cout << "success = " << result << ")\n"; }
#endif
        return result;
    }

private:
    // removes a positive sign in front of the literal due to requirements of used underlying
    // [from_chars](https://en.cppreference.com/w/cpp/utility/from_chars):
    // "the plus sign is not recognized outside of the exponent" - for VHDL's integer exponent
    // this is allowed, and it's a valid rule for outer parser!
    static std::string_view remove_positive_sign(std::string_view literal) {
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
static const from_chars_api<TargetT> from_chars = {};

template <typename TargetT>
static inline auto as_unsigned(std::string_view literal, unsigned base, std::error_code& ec)
{
    static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                  "TargetT must be of unsigned integral type");

    auto const clean_literal = convert::detail::remove_underline(literal);
    return from_chars<TargetT>(clean_literal, base, ec);
}

template <typename TargetT>
static inline auto as_real(std::string_view literal, unsigned base, std::error_code& ec)
{
    static_assert(std::is_floating_point_v<TargetT>, "TargetT must be of floating point type");

    //FIXME Implementation; why isn't remove_underline() here?
    //assert(base == 10U && "only base 10 for real type is supported");
    return TargetT{42}; // FIXME, temporary for test

    return from_chars<TargetT>(literal, base, ec);
}

// concept, see [coliru](https://coliru.stacked-crooked.com/a/09a6475cf60dd1e9)
// concept, part #2: https://coliru.stacked-crooked.com/a/47b7cc5e62eb7431
// concept, part #3: https://godbolt.org/z/oKTP5ajWb
template <typename T, unsigned Base>
class exp_scale {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "T must be integral type");
    static_assert(Base > 1 && Base < 37, "Base must be in range [2,36]");

public:
    using value_type = T;

public:
    constexpr exp_scale()
    {
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
    static constexpr auto SIZE = convert::detail::digits_traits<T,  Base>::value;
    std::array<value_type, SIZE> data;
};

template <typename T>
static constexpr exp_scale<T, 10U> exp10_scale = {};

}  // namespace detail

template <typename TargetT>
struct convert_real {
    static_assert(std::is_floating_point_v<TargetT>, "TargetT must be of floating point type");

    std::optional<TargetT> operator()(ast::real_type const& real, std::error_code& ec) const
    {
        std::cout << "convert_real '" << real << "'\n";

        //FIXME assert(base == 10U && "only base 10 for real type is supported");

        // FixMe: The approach joining real parts into single one does work
        // only for decimal real, with e.g. hex the literal 16#AFFE_1.0Cafe#e-10
        // becomes AFFE_1.0Cafee-10, which is wrong!

        auto const real_string = as_real_string(real);
        auto const real_result = detail::as_real<TargetT>(real_string, real.base, ec);

        if (ec) {
            return {};
        }

        return real_result;
    }

private:
    // join all parsed elements and prune delimiter '_' to be prepared for indirect call
    // of `from_chars()`. The result is a C++ standard conformance float string.
    // Concept: @see [godbolt.org](https://godbolt.org/z/KroM7oxc5)
    // @note Care must be taken on stack-use-after-scope
    // ([godbolt](https://godbolt.org/z/vr39h7Y8d)) but not relevant since std::string is returned.
    std::string as_real_string(ast::real_type const& real) const
    {
        using namespace std::literals::string_view_literals;

        auto const remove_underline = [](auto const& literal_list) {
            namespace views = ranges::views;
            return ranges::to<std::string>(literal_list | views::join |
                                           views::filter(detail::underline_predicate));
        };

        auto const as_sv = [](std::string const& str) { return std::string_view(str); };

        if (real.exponent.empty()) {
            // clang-format off
            auto const literal_list = {
                as_sv(real.integer), "."sv, as_sv(real.fractional)
            };
            // clang-format on
            return remove_underline(literal_list);
        }

        // clang-format off
        auto const literal_list = {
            as_sv(real.integer), "."sv, as_sv(real.fractional), "e"sv, as_sv(real.exponent)
        };
        // clang-format on

        return remove_underline(literal_list);
    }
};

template <typename TargetT>
static convert_real<TargetT> const real = {};

template <typename TargetT>
struct convert_integer {
    static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                  "TargetT must be of unsigned integral type");

    std::optional<TargetT> operator()(ast::integer_type const& integer, std::error_code& ec) const
    {
        std::cout << "convert_integer '" << integer << "'\n";

        auto const int_result = detail::as_unsigned<TargetT>(integer.integer, integer.base, ec);

        if (ec) {
            return {};
        }

        if (integer.exponent.empty()) {
            // nothings more to do
            return int_result;
        }

        if(integer.base == 10U) {

            // transform e.g. "nnnE6" to index of 6 for lookup of base 10
            auto const exp10_index = detail::as_unsigned<TargetT>(integer.exponent, integer.base, ec);

            if (ec) {
                // error from std::from_chars()
                return {};
            }

            static auto constexpr exp10_scale = detail::exp10_scale<TargetT>;

            if (!(exp10_index < exp10_scale.max_index())) {
                // exponent 10^index out of range or others
                ec = std::make_error_code(std::errc::value_too_large);
                return {};
            }

            // scale the integer with exponent; may overkill here: [Safe Numerics](
            //  https://www.boost.org/doc/libs/develop/libs/safe_numerics/doc/html/index.html)
            // hence we use our own multiplication to detect overflows.
            TargetT const result = ::util::mul<TargetT>(int_result, exp10_scale[exp10_index], ec);

            if (ec) {
                // numeric range overflow
                return {};
            }

            return result;
        }
        else {
            std::cerr << "FixMe: convert_integer '" << integer << " (base != 10)\n";
        }
        return 666;
    }
};

template <typename TargetT>
static convert_integer<TargetT> const integer = {};

// --------------------------------------------------------------------------------

// Note, no template argument, it's not expected to switch the implementation based
// on numeric Base.
template <typename TargetT>
struct convert_bit_string_literal {
    static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                  "TargetT must be of unsigned integral type");

    std::optional<TargetT> operator()(ast::bit_string_literal const& literal, std::error_code& ec) const
    {
        auto const digit_string = detail::remove_underline(literal.literal);
        auto const binary_number = detail::from_chars<TargetT>(digit_string, literal.base, ec);

        if (ec) {
            return {};
        }

        return binary_number;
    }
};

template <typename TargetT>
static convert_bit_string_literal<TargetT> const bit_string_literal = {};

}  // namespace convert
