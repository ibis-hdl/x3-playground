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

template <typename Iterator>
struct numeric_failure : x3::expectation_failure<Iterator> {
public:
    numeric_failure(leaf::e_position_iterator<Iterator> const& where, std::string const& which,
                    const std::string& what_arg)
        : x3::expectation_failure<Iterator>(where.where, which)
        , what_{ what_arg }
    {
    }

    ~numeric_failure() {}

    virtual const char* what() const noexcept override { return what_.c_str(); }

private:
    std::string const what_;
};

}  // namespace convert
