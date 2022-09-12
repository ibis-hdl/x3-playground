//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/convert/leaf_errors.hpp>

#include <cfenv>
#include <string_view>
#include <string>
#include <iostream>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/ostream.h>

#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/reverse.hpp>

template <> struct fmt::formatter<boost::leaf::e_error_trace::record>;

namespace boost::leaf {

std::ostream& operator<<(std::ostream& os, e_fp_exception e_fp) {
    fmt::print(os, "{}", e_fp.as_string());
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

    static constexpr auto fp_exception_name = [](int fp_raised) {
        if (fp_raised & FE_OVERFLOW)  { return "overflow"sv; }
        if (fp_raised & FE_UNDERFLOW) { return "underflow"sv; }
        if (fp_raised & FE_INVALID)   { return "invalid"sv; }
        if (fp_raised & FE_DIVBYZERO) { return "division-by-zero"sv; }
        if (fp_raised & FE_INEXACT)   { return "inexact"sv; }
        return std::string_view{};
    };

    auto /* const */ fe_list = exceptions
        | views::transform([&](int e){ return fp_exception_name(e & raised); })
        | views::filter([](auto sv){ return !sv.empty(); })
        ;
    // clang-format on

    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf), "{}", fmt::join(fe_list, ", "));

    return to_string(buf);
}


std::ostream& operator<<(std::ostream& os, e_error_trace const& trace)
{
    namespace views = ranges::views;

    for(auto i = trace.value.size(); auto const& record : trace.value | views::reverse) {
        fmt::print(os, "  {}: {}\n", i--, record);
    }
    return os;
}

}  // namespace boost::leaf

//
// {fmt} lib formatter
// 

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
