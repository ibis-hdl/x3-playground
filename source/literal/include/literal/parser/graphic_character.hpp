//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

namespace parser {

namespace x3 = boost::spirit::x3;

// TODO Check approach of
// [How to define a spirit x3 parser that parses a single char from any encoding spaces?](
// https://stackoverflow.com/questions/42930438/how-to-define-a-spirit-x3-parser-that-parses-a-single-char-from-any-encoding-spa)

static auto const graphic_character = x3::graph | x3::space;

using graphic_character_type = std::remove_cvref_t<decltype(graphic_character)>;

}  // namespace parser

namespace boost::spirit::x3 {

template <>
struct get_info<::parser::graphic_character_type> {
    using result_type = std::string;
    std::string operator()([[maybe_unused]] ::parser::graphic_character_type const&) const
    {
        return "graphic character";
    }
};

}  // namespace boost::spirit::x3
