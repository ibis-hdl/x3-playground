//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/detail/digit_traits.hpp>
#include <literal/detail/constraint_types.hpp>

#include <string>
#include <string_view>
#include <limits>
#include <cmath>

#include <iostream>
#include <iomanip>

namespace convert {

namespace detail {

// concept, see [coliru](https://coliru.stacked-crooked.com/a/09a6475cf60dd1e9)
// concept, part #2: https://coliru.stacked-crooked.com/a/47b7cc5e62eb7431
// concept, part #3: https://godbolt.org/z/oKTP5ajWb
template <UnsignedIntergralType IntT, unsigned Base> requires BasicBaseRange<Base>
class power_table
{
public:
    using value_type = IntT;

    enum size_t {
        MAX_INDEX = convert::detail::digits_traits_v<value_type,  Base>
    };

public:
    constexpr power_table()
    {
        value_type value = 1;
        for (std::size_t i = 0; i != MAX_INDEX; ++i) {
            data[i] = value;
            value *= Base;
        }
    }

    value_type operator[](unsigned idx) const
    {
        assert(idx < MAX_INDEX && "exponent index out of range");
        return data[idx];
    }

    static unsigned max_index() { return MAX_INDEX; }

private:
    std::array<value_type, MAX_INDEX> data;
};

template<typename T>
struct power_fu {
    static_assert(nostd::always_false<T>, "Muts be of unsigned integer or real type");
};

template<typename T>
static power_fu<T> const power = {};

template<UnsignedIntergralType IntT>
struct power_fu<IntT>
{
    IntT operator()(unsigned base, unsigned exp_index, std::error_code& ec) const
    {
        auto const max_exp = [&](unsigned base) {
            switch(base) {
                case 2:
                    return bin_lut.max_index();
                case 8:
                    return oct_lut.max_index();
                case 10:
                    return dec_lut.max_index();
                case 16:
                    return hex_lut.max_index();
                default: {
                    // Generic implementation for arbitrary bases
                    std::cerr << "WARNING: use of weak supported base of " << base << '\n';
                    auto const log = [](double base, double x) {
                        return std::log10(x) / std::log10(base);
                    };
                    // ToDo: Maybe cache the value by base as map/vector for search/reuse
                    return static_cast<unsigned>(std::numeric_limits<IntT>::digits / log(2, base));
                }
            }
        };

        if (!(exp_index < max_exp(base))) {
            // exponent base^index out of range or others
            ec = std::make_error_code(std::errc::value_too_large);
            return {};
        }

        return (*this)(base, exp_index);
    }

private:
    IntT operator()(unsigned base, unsigned exp) const
    {
        auto const get = [&](unsigned base, unsigned e) {
            switch(base) {
                case 2:
                    return bin_lut[e];
                case 8:
                    return oct_lut[e];
                case 10:
                    return dec_lut[e];
                case 16:
                    return hex_lut[e];
                default:
                    // TODO check on max exponent
                    return static_cast<IntT>(std::pow(static_cast<double>(base), static_cast<double>(e)));
            }
        };

        return get(base, exp);
    }

private:
    static constexpr auto bin_lut = power_table<IntT,  2U>{};
    static constexpr auto oct_lut = power_table<IntT,  8U>{};
    static constexpr auto dec_lut = power_table<IntT, 10U>{};
    static constexpr auto hex_lut = power_table<IntT, 16U>{};
};

template<RealType RealT>
struct power_fu<RealT>
{
    RealT operator()(unsigned base, std::int32_t exp_index, std::error_code& ec) const
    {
        // FIXME make it configurable/templated
        using promote_type = std::uint32_t;

        // TODO Check on max base and exponent

        auto const power_int = [&](unsigned base, std::int32_t exp)
        {
            auto const exp_ = abs(exp);

            // check if exponent small enough so one can use integer table lookup
            if (exp_ <= std::numeric_limits<promote_type>::digits10) {
                auto const result = static_cast<RealT>(power<promote_type>(base, exp_, ec));
                if(exp > 0) {
                    return result;
                }
                // otherwise exponent negative
                assert(result > 0 && "attempt to call division by zero"); // shouldn't happen - paranoia
                return RealT{1} / result;
            }
            else { // > digits10
                return std::pow(static_cast<RealT>(base), static_cast<RealT>(exp));
            }
        };

        return power_int(base, exp_index);
    }
};

}  // namespace detail

}  // namespace convert