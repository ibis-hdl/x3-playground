//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/convert_error.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

#include <boost/type_index.hpp>

#include <fmt/format.h>
#include <string>
#include <string_view>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace {

// [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
// https://coliru.stacked-crooked.com/a/7fd3fbc8b2452590
// for different error recovery strategies
template <typename RuleID>
struct my_x3_error_handler {  // try to recover and continue to parse
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& e, Ctx const& ctx) const
    {
        // This function must catch the `x3::expectation_failure` exceptions (see
        // [expect_directive](
        // https://github.com/boostorg/spirit/blob/master/include/boost/spirit/home/x3/directive/expect.hpp))
        // and the exceptions from the numerical conversion. From the X3 design, only the
        // exceptions of the type `x3::expectation_failure` are passed on from the [guard](
        // https://github.com/boostorg/spirit/blob/master/include/boost/spirit/home/x3/auxiliary/guard.hpp)
        // to the error handler. Therefore, own ones must be derived from them and dynamically
        // up-casted within the handler to be able to distinguish between them. In this way, more
        // descriptive error messages can be elaborated - the actual purpose. Concept: @see
        // [coliru](https://coliru.stacked-crooked.com/a/4163d57183f76f93)
        auto const error_message = [](auto const& e) {

            using convert_exception_pointer = ::convert::numeric_failure<It> const*;

            if (auto e_ptr = dynamic_cast<convert_exception_pointer>(&e); e_ptr != nullptr) {
                return fmt::format(
                    "Error '{}' during numerical conversion while parsing '{}' here:",
                    e_ptr->what(), e_ptr->which());
            }
            else {
                // covers x3::expectation_failure
                return fmt::format("Error! Expecting {} here:", e.which());
            }
        };

        std::cerr << fmt::format("### error handler for RuleID: '{}' ###\n",
                                 boost::typeindex::type_id<RuleID>().pretty_name());
        auto& error_handler = x3::get<x3::error_handler_tag>(ctx);
        error_handler(e.where(), error_message(e));

        // Error strategy: Accept the mistakes so far. They are already reported before. One
        // point to recover the parser from errors is to find a semicolon that terminates an
        // expression.
        // FIXME Doesn't work as intended - doesn't recover correctly: failed with expectation
        // error of ';'
        auto const position = std::string_view(first, last).find_first_of(";");
        if (position != std::string_view::npos) {
            std::advance(first, position + 1);  // move iter behind
            return x3::error_handler_result::accept;
        }

        first = last;  // no way
        return x3::error_handler_result::fail;
    }
};

}  // namespace