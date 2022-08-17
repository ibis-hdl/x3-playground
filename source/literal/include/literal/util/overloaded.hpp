//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace util {

// [cppreference.com std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
// helper type for the visitor
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20), but for Clang (14)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace util
