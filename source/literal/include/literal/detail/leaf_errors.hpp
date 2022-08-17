//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/leaf.hpp>
#include <boost/spirit/home/x3/core/parser.hpp> // x3::what()
#include <fmt/format.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <iostream>

// This header exist to avoid collisions and compiler errors between spirit X3 includes
// and project's one. The namespace is also leaf to be used project wide and since it's
// tied to boost LEAF error handling respectively extends the error IDs.

namespace boost::leaf {

template <typename IterT>
struct e_position_iterator {
    IterT const value;  // mimics x3::expectation_failure::where()
    // clang doesn't support parentheses init syntax for aggregate init (C++20 addition)
    explicit e_position_iterator(IterT iter) : value{ iter }
    {}
};


template<typename IterT>
struct e_convert_parser_context
{
public:
    e_convert_parser_context() = delete;
    ~e_convert_parser_context() = default;

    e_convert_parser_context(e_convert_parser_context const&) = delete;
    e_convert_parser_context& operator=(e_convert_parser_context const&) = delete;

    e_convert_parser_context(e_convert_parser_context&&) = default;
    e_convert_parser_context& operator=(e_convert_parser_context&&) = default;

    e_convert_parser_context(std::string what_, IterT first_, IterT begin_)
    : x3_what{ what_}
    , first{ first_}
    , begin{begin_}
    {}

public:
    void unroll() {
        std::cerr << "restore parser iterator position\n";
        first = begin;
    }

    IterT iter() const { return first; }
    std::string which() const { return x3_what; }

private:
    std::string const x3_what;
    IterT first;
    IterT const begin;
};


struct e_fp_exception {
    int const raised;
    std::string as_string() const;
};

}  // namespace boost::leaf
