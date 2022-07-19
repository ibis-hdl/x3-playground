//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/leaf.hpp>

#include <fmt/format.h>

#include <string>
#include <iostream>

// This header exist to avoid collisions and compiler errors between spirit X3 includes
// and project's one. The namespace is also leaf to be used project wide and since it's
// tied to boost LEAF error handling respectively extends the error IDs.

namespace boost::leaf {

template <typename IterT>
struct e_position_iterator {
    e_position_iterator(IterT where_)
        : where{ where_ }
    {
    }
    IterT const where;  // mimics x3::expectation_failure::where()
};

struct e_fp_exception {
    int const raised;
    std::string as_string() const;
};

}  // namespace boost::leaf
