//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <type_traits>

// TODO rename to concept types

// Note from [cpplang slack *concept* channel](
// https://cpplang.slack.com/archives/C6K47F8TT/p1655242903316339) "std integral is just
// the set of fundamental integer types, no library type is able to model the concept. " ... "Looks
// like libraries relying on std::is_integral simply cannot deal with UDT number types. They can
// only hope to by relying on numer_limits<T>::is_integer instead."
//
// This affects the header <../int_types.hpp> where boost::multiprecision is used for
// 128-bit integer for use with MSVC!

template <class IntT>
concept IntegralType = std::is_integral_v<IntT> && !std::is_same_v<IntT, bool>;

template <class IntT>
concept UnsignedIntegralType = std::is_unsigned_v<IntT> && IntegralType<IntT>;

template <class IntT>
concept SignedIntegralType = std::is_signed_v<IntT> && IntegralType<IntT>;

template <class RealT>
concept RealType = std::is_floating_point_v<RealT>;

template <unsigned Base>
concept BasicBaseRange = (2 <= Base && Base <= 36);

template <unsigned Base>
concept SupportedBase = (Base == 2 || Base == 8 || Base == 10 || Base == 16);

namespace nostd {

///
/// Small helper for self explaining compiler error messages
///
/// Usage, e.g.:
/// @code {.cpp}
/// if constexpr (std::is_floating_point_v<T>) {
///     // handle this case
/// }
/// else {
///     static_assert(always_false<T>, "type not supported");
/// }
/// @endcode
///
template <class... T>
constexpr bool always_false = false;

}  // namespace nostd
