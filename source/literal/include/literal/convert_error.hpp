//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/spirit/home/x3/directive/expect.hpp>  // x3::expectation_failure

#include <literal/detail/leaf_errors.hpp>

#include <string>

namespace convert {

namespace x3 = boost::spirit::x3;
namespace leaf = boost::leaf;

///
/// Exception class for convert numeric errors.
///
/// @tparam IteratorT Parser iterator type
///
template <typename IteratorT>
struct numeric_failure : x3::expectation_failure<IteratorT>
{
public:
    numeric_failure() = delete;
    numeric_failure(numeric_failure const&) = default;
    numeric_failure(numeric_failure&&) = default;
    ~numeric_failure() = default;

    numeric_failure& operator=(numeric_failure const&) = delete;
    numeric_failure& operator=(numeric_failure&&) = default;

public:
    numeric_failure(IteratorT where, std::string const& which, std::string const& what_str)
        : x3::expectation_failure<IteratorT>(where, which)
        , what_{ what_str }
    {
    }

    const char* what() const noexcept override { return what_.c_str(); }

private:
    std::string const what_;
};

}  // namespace convert
