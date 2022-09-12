//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/parser/error_handler.hpp>

struct based_literal_class : parser::my_x3_error_handler<based_literal_class> {};
struct decimal_literal_class : parser::my_x3_error_handler<decimal_literal_class> {};
struct bit_string_literal_class : parser::my_x3_error_handler<bit_string_literal_class> {};
struct character_literal_class : parser::my_x3_error_handler<character_literal_class> {};
struct string_literal_class : parser::my_x3_error_handler<string_literal_class> {};
struct literal_rule_class : parser::my_x3_error_handler<literal_rule_class> {};
struct grammar_class : parser::my_x3_error_handler<grammar_class> {};
