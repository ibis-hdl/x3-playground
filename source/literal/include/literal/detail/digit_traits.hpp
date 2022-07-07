// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <boost/spirit/home/x3/support/numeric_utils/detail/extract_int.hpp>


namespace convert {

namespace detail {

///
/// The maximum radix digits that can be represented without overflow
///
/// @tparam T Underlying type
/// @tparam Radix Radix of interest represented in decimal system
///
/// @note Simple derived from Spirit X3 for convenience
///
/// Usage, e.g.:
/// @code{.cpp}
/// assert(std::string_view{ "123456789" }.size() <= (digits_traits<std::uint32_t, 2>::value));
/// @endcode
///
template <typename T, unsigned Radix>
struct digits_traits : boost::spirit::x3::detail::digits_traits<T, Radix> {};

} }

