//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/ast.hpp>

#include <string>

bool parse(std::string const& input, ast::literals& literals, std::ostream& os);

void reset_error_counter();
