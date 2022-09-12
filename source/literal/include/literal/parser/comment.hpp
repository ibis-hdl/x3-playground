//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3.hpp>

namespace parser {

namespace x3 = boost::spirit::x3;

static auto const c_style_comments = "/*" >> x3::lexeme[*(x3::char_ - "*/")] >> "*/";
static auto const cpp_style_comment = "//" >> *~x3::char_("\r\n");
static auto const comment = cpp_style_comment | c_style_comments;

} // namespace parser
