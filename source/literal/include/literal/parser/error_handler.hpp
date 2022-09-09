//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/convert/numeric_failure.hpp>
#include <literal/parser/comment.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

#include <boost/type_index.hpp>

#include <fmt/format.h>

#include <string_view>
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace parser {
namespace detail {

///
/// Get an excerpt as `string_view` of the iterator range and pay attention 
/// to the iterator end to avoid reading from invalid memory ranges.
///
auto const excerpt_sv = [](auto first, auto last) {
    auto const sz = std::min(15U, static_cast<unsigned>(std::distance(first, last)));
    return (first == last) ? "<eoi>" : std::string_view{first, first + sz};
};

///
/// Default error strategy: Accept the mistakes so far.
///
/// The error message must have been previously reported.
///
/// @note A simple search for a point of recovery, e.g.
/// @code{.cpp}
/// if (auto semicolon = std::find(first, last, ';'); semicolon == last) {
///     return x3::error_handler_result::fail;
/// } else {  // move iter behind ';'
///     first = semicolon + 1;
///     return x3::error_handler_result::retry;
/// }
/// @endcode 
/// doesn't work due to comments in the input!
///
template <typename RuleID, typename SymbolT>
struct error_recovery_strategy {

    explicit error_recovery_strategy(SymbolT symbol_)
    : symbol{ symbol_ }
    {}

    template <typename It, typename Ctx>
    bool operator()(It& first, It last, [[maybe_unused]] Ctx const& ctx) const {

        auto constexpr verbose = false;

        if constexpr(verbose) {
            static auto const find_grammar = x3::skip(x3::space | parser::comment)[
                x3::lexeme[ +(x3::char_ - symbol) >> x3::char_(symbol) ]
            ];
            std::cout << "+++ recover in: '" << excerpt_sv(first, last) << " ...'\n";
            std::string skip_over_str;
            if(x3::parse(first, last, find_grammar, skip_over_str)) {
                std::cout << "+++ recover skip: '" << skip_over_str << " ...'\n";
                return true;
            }
        }
        else {
            static auto const find_grammar = x3::skip(x3::space | parser::comment)[
                +(x3::char_ - symbol) >> symbol
            ];
            if(x3::parse(first, last, find_grammar)) {
                return true;
            }
        }

        // no way ...
        return false;
    }

private:
    SymbolT const symbol;
};

template <typename RuleID>
struct error_recovery {

    template <typename It, typename Ctx>
    bool operator()(It& first, It last, Ctx const& ctx) const {
        return strategy(first, last, ctx);
    }

private:
    static inline error_recovery_strategy<RuleID, char> const strategy{';'};
};

} // namespace detail

///
/// syntactic sugar for specialized parser error recovery strategies.
///
/// It's using error strategy customization points, for concept
/// - https://godbolt.org/z/xrGqx557E
/// - https://godbolt.org/z/65rrn5qKx (alternative (constexpr) approach)
///
template <typename RuleID, typename It, typename Ctx>
bool error_recovery(It& first, It last, Ctx const& ctx) {
    return detail::error_recovery<RuleID>{}(first, last, ctx);
}

/// 
/// Customizable parser error handler to use different error recovery strategies
/// 
/// The general concept of taggable error handler is based on 
/// [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
/// It is using several auxillary classes to try to recover from parser errors.
///
/// [Slack](https://cpplang.slack.com/archives/C27KZLB0X/p1662738072331189):
/// - @see https://godbolt.org/z/4qEfrP6GK (strong simplified, but works)
/// - @see https://godbolt.org/z/W8GvrY1qv (Sehe's reduction)
///
/// FIXME doesn't recover as intended, see [Slack](https://cpplang.slack.com/archives/C27KZLB0X/p1662738072331189)
///
template <typename RuleID>
struct my_x3_error_handler {  // try to recover and continue to parse
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& e, Ctx const& ctx) const
    {
        auto constexpr verbose = false;

        auto const error_message = [](auto const& e) {
            // This function must catch the `x3::expectation_failure` exceptions and the exceptions
            // from the numerical conversion. From the X3 design, only the exceptions of the type 
            // `x3::expectation_failure` are passed on from e.g. [parse_rhs_main()](
            // https://github.com/boostorg/spirit/blob/master/include/boost/spirit/home/x3/nonterminal/detail/rule.hpp)
            // to the error handler. Therefore, own ones must be derived from them and dynamically
            // up-casted within the handler to be able to distinguish between them. In this way, more
            // descriptive error messages can be elaborated - the actual intend. Concept:
            // @see [Coliru](https://coliru.stacked-crooked.com/a/4163d57183f76f93)
            using pointer_type = typename ::convert::numeric_failure<It>::const_pointer_type;

            if (auto e_ptr = dynamic_cast<pointer_type>(&e); e_ptr == nullptr) {
                // aka x3::expectation_failure
                return fmt::format("Error! Expecting {} here:", e.which());
            }
            else {
                return fmt::format("Error '{}' in the numerical conversion of '{}' here:",
                    e_ptr->what(), e_ptr->which());
            }
        };

        if constexpr(verbose) {
            std::cerr << fmt::format(">>> error handler for RuleID: <{}> <<<\n", id());
        }

        auto const& error_handler = x3::get<x3::error_handler_tag>(ctx);
        error_handler(e.where(), error_message(e));

        if(error_recovery<RuleID>(first, last, ctx)) {
            if constexpr(verbose) {
                // pre-skip, but it's more cosmetically
                x3::skip_over(first, last, ctx);
                std::cout << "+++ ignoring errors and continue with:\n'" 
                          << detail::excerpt_sv(first, last) << " ...'\n";
            }
            return x3::error_handler_result::retry;
        }

        if constexpr(verbose) {
            std::cout << "+++ give up on errors\n";
        }
        return x3::error_handler_result::fail;
    }

private:
    static std::string id() {
        return boost::core::demangle(typeid(RuleID).name());
    }

};

}  // namespace namespace parser
