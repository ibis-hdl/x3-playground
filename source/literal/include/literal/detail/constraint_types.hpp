//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <type_traits>

template <class IntT>
concept IntergralType = std::is_integral_v<IntT> && !std::is_same_v<IntT, bool>;

template <class IntT>
concept UnsignedIntergralType = std::is_unsigned_v<IntT> && IntergralType<IntT>;

template <class IntT>
concept signedIntergralType = std::is_signed_v<IntT> && IntergralType<IntT>;

template <class RealT>
concept RealType = std::is_floating_point_v<RealT>;

template<unsigned Base>
concept BasicBaseRange = (2 <= Base && Base <= 36);

template<unsigned Base>
concept SupportedBase = (Base == 2 || Base == 8 || Base == 10 || Base == 16);

namespace nostd {

///
/// Small helper for self explaining compiler error messages
///
/// Usage:
/// @code {.language-id}
/// static_assert(always_false<T>, "must use correct specialization");
/// @endcode
///
/// or even:
/// @code {.language-id}
/// if constexpr (std::is_floating_point_v<T>) {
///     // handle this case
/// }
/// else {
///     static_assert(always_false<T>, "T not supported");
/// }
/// @endcode
///
template <class... T>
constexpr bool always_false = false;

}
