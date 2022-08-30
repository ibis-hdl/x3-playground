//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/detail/leaf_errors.hpp>

#include <cfenv>
#include <string_view>
#include <string>
#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <range/v3/all.hpp>

template <> struct fmt::formatter<boost::leaf::e_error_trace::record> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(boost::leaf::e_error_trace::record const& rec, FormatContext& ctx) const -> decltype(ctx.out()) {
        fmt::format_to(ctx.out(), "{}({})", rec.file, rec.line);
        return ctx.out();
    }
};

namespace boost::leaf {

std::ostream& operator<<(std::ostream& os, e_fp_exception e_fp) {
    os << e_fp.as_string();
    return os;
}

std::string e_fp_exception::as_string() const
{
    // concept see [godbolt](https://godbolt.org/z/qffrdcjbx)
    using namespace std::literals::string_view_literals;
    namespace views = ranges::views;

    // clang-format off
    static constexpr auto exceptions = {
        // see https://en.cppreference.com/w/c/numeric/fenv/FE_exceptions
        FE_OVERFLOW, 
        FE_UNDERFLOW, 
        FE_INVALID, 
        FE_DIVBYZERO, 
        FE_INEXACT 
    };

    static constexpr auto fp_exception_name = [](int raised) {
        if (raised & FE_OVERFLOW)  { return "overflow"sv; }
        if (raised & FE_UNDERFLOW) { return "underflow"sv; }
        if (raised & FE_INVALID)   { return "invalid"sv; }
        if (raised & FE_DIVBYZERO) { return "division-by-zero"sv; }
        if (raised & FE_INEXACT)   { return "inexact"sv; }
        return std::string_view{};
    };

    auto const str = exceptions
        | views::transform([&](int e){ return fp_exception_name(e & this->raised); })
        | views::filter([](auto sv){ return !sv.empty(); }) 
        | views::join(", ")
        | ranges::to<std::string>()
        ; 
    // clang-format on

    return str;
}


std::ostream& operator<<(std::ostream& os, e_error_trace const& trace) 
{
    for(unsigned i = 1; auto const& rec : trace.value) {
        fmt::print(os, "  {}: {}\n", i++, rec);
    }
    return os;
}

}  // namespace boost::leaf
