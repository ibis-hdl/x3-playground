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
#include <functional>
#include <concepts>
#include <deque>

// Common errors tied to boost LEAF error handling respectively extends the error IDs.

namespace boost::leaf {

namespace x3 = boost::spirit::x3;

template <typename IterT>
struct e_position_iterator {
    IterT const value;  // mimics x3::expectation_failure::where()
    // clang doesn't support parentheses init syntax for aggregate init (C++20 addition)
    explicit e_position_iterator(IterT iter)
    : value{ iter }
    {}
};


template<typename D>
concept X3ParserT = std::derived_from<D, x3::parser<D>>;

///
/// The call of x3::what() can be made lazy during construction time of
/// the class, see [coliru](https://coliru.stacked-crooked.com/a/554f65ab2bd8e7b7).
/// This simplifies the LEAF on-error payload:
/// @code{.cpp}
/// auto load = leaf::on_error([&]{ // lazy for call of x3::what()
///     return leaf::e_x3_parser_context{x3::what(*this), first, first_bak}; });
/// @endcode
///
template<typename IterT>
struct e_x3_parser_context
{
public:
    e_x3_parser_context() = delete;
    ~e_x3_parser_context() = default;

    e_x3_parser_context(e_x3_parser_context const&) = delete;
    e_x3_parser_context& operator=(e_x3_parser_context const&) = delete;

    e_x3_parser_context(e_x3_parser_context&&) = default;
    e_x3_parser_context& operator=(e_x3_parser_context&&) = default;

    ///
    /// Construct the parser error context for error handling using LEAF.
    ///
    /// @param x3_what The Spirit X3 parser name which fails, by `x3::which()`
    /// @param first_ Iterator position pointing to end of parsers expression.
    /// @param first_bak_ Iterator backup to be restored if intended.
    ///
    e_x3_parser_context(std::string const& x3_what, IterT first_, IterT first_bak_)
    : x3_what_str{ x3_what }
    , first{ first_}
    , first_bak{ first_bak_ }
    {}

    ///
    /// Construct the parser error context for error handling using LEAF, calls
    /// `x3::which()` lazy.
    ///
    /// @tparam ParserT Spirit X3 parser type.
    /// @param parser The Spirit X3 parser which fails.
    /// @param first_ Iterator position pointing to end of parsers expression.
    /// @param first_bak_ Iterator backup to be restored if intended.
    ///
    template<X3ParserT ParserT>
    e_x3_parser_context(ParserT const& parser, IterT first_, IterT first_bak_)
    : x3_what_lazy_fn{ [this, &parser](){ return x3::what(parser); } }
    , first{ first_}
    , first_bak{ first_bak_ }
    {}

public:
    /// Restore the iterator to the position before the error occurred.
    void unroll() {
        std::cerr << "Restore parser iterator position\n";
        first = first_bak;
    }

    /// The iterator pointing to the erroneous position.
    IterT iter() const { return first; }

    /// The name of the parser which fails, @see x3::what()
    std::string which() const {
        if(x3_what_lazy_fn) {
            x3_what_str = x3_what_lazy_fn();
        }
        return x3_what_str;
    }

private:
    std::function<std::string()> const x3_what_lazy_fn;
    std::string mutable x3_what_str{ "N/A" };
    IterT first;
    IterT const first_bak;
};


struct e_fp_exception {
    int const raised;
    friend std::ostream& operator<<(std::ostream& os, e_fp_exception e_fp);
    std::string as_string() const;
};

///
/// The error trace is activated only if an error handling scope provides a
/// handler for e_error_trace.
/// @see [LEAF](https://github.com/boostorg/leaf/blob/develop/example/error_trace.cpp)
//
struct e_error_trace
{
    struct record {
        char const * file;
        int line;
    };

    friend std::ostream& operator<<(std::ostream& os, e_error_trace const& trace);

    std::deque<record> value;
};

}  // namespace boost::leaf

#define LEAF_ERROR_TRACE auto leaf_trace__ = ::boost::leaf::on_error([](::boost::leaf::e_error_trace& trace) { trace.value.emplace_front(::boost::leaf::e_error_trace::record{__FILE__, __LINE__}); } )
