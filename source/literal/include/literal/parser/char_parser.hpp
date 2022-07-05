//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

#include <string>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace char_parser {

auto const delimit_numeric_digits = [](auto&& char_range, char const* name) {
    auto cs = x3::char_(char_range);
    // clang-format off
    return x3::rule<struct _, std::string>{ "numeric digits" } =
        x3::raw[cs >> *('_' >> +cs | cs)];
    // clang-format on
};

auto const bin_digits = delimit_numeric_digits("01", "binary digits");
auto const oct_digits = delimit_numeric_digits("0-7", "octal digits");
auto const dec_digits = delimit_numeric_digits("0-9", "decimal digits");
auto const hex_digits = delimit_numeric_digits("0-9a-fA-F", "hexadecimal digits");

}  // namespace char_parser
}  // namespace parser
