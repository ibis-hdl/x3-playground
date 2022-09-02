//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// macOS Clang users depend on libc++, where std::from_chars isn't
// implemented for float types
#if defined(_LIBCPP_VERSION)
#define CONVERT_FROM_CHARS_USE_STRTOD
#endif

// define this to test strtod() implementation of std::from_chars()
//#define CONVERT_FROM_CHARS_USE_STRTOD
