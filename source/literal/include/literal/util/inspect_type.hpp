//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/core/demangle.hpp>
#include <boost/type_index.hpp>

namespace util {

template <typename Expr>
std::string inspect_type(Expr const&)
{
#if 1
    return boost::core::demangle(typeid(Expr).name());
#else
    using A = typename x3::traits::attribute_of<Expr, x3::unused_type>::type;
    return boost::core::demangle(typeid(A).name());
#endif
}

#if 0
template <typename T>
void the_type()
{
    // char const* name = typeid(T).name()
    // std::cout << boost::core::demangle( name ) << '\n';
    std::cout << boost::typeindex::type_id<T>().pretty_name() << "\n";
}
#endif

}  // namespace util
