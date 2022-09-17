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

#include <fmt/format.h>

#include <string_view>
#include <algorithm>
#include <typeindex>
#include <iostream>
#include <iomanip>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace detail {

/// global developer settings for verbose/noisy debugging of error recovery and handling
static bool constexpr verbose_error_handler = false;

///
/// Get an excerpt as `string_view` of the iterator range and pay attention
/// to the iterator end to avoid reading from invalid memory ranges. Intended to be
/// used during developing and debugging parser error strategies only.
///
auto const excerpt_sv = [](auto first, auto last) {
    std::size_t const sz = std::min(25U, static_cast<unsigned>(std::distance(first, last)));
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 13000
    return (first == last) ? "<eoi>" : std::string_view(&*first, sz);
#else
    return (first == last) ? "<eoi>" : std::string_view{first, first + sz};
#endif
};

struct error_recovery_strategy_map {
    using lookup_result = struct {
        char symbol;
        x3::error_handler_result handler_result;
    };
    static lookup_result lookup(std::type_index id);
};

template<typename RuleID>
auto error_recovery_strategy_aux() {
    return error_recovery_strategy_map::lookup(std::type_index(typeid(RuleID)));
}

struct error_recovery_common_strategy {

    using result_type = std::tuple<bool, x3::error_handler_result>;
    using iterator_type = std::string::const_iterator;

    static result_type call(iterator_type& first, iterator_type last,
                            error_recovery_strategy_map::lookup_result const& recovery_aux);
};

///
/// Default error strategy: Accept the mistakes so far.
///
/// The error message must have been previously reported.
///
/// The concrete strategy depends on the used spirit rule (e.g. top level rule etc.).
/// The selection of `x3::error_handler_result`depends on the concrete use case:
/// - `accept` proceeds as if the current rule passed.
/// - `retry` proceeds as if the current rule never started
/// - `rethrow` as the name says, delegates the error to outer error handler.
///
/// If the default doesn't work one can provide a (full) specialization.
///
/// @note A simple search for a point of recovery doesn't work due to comments in the
///       input!, e.g.
/// @code{.cpp}
/// if (auto semicolon = std::find(first, last, ';'); semicolon == last) {
///     return x3::error_handler_result::fail;
/// } else {  // move iter behind ';'
///     first = semicolon + 1;
///     return x3::error_handler_result::retry;
/// }
/// @endcode
///
template <typename RuleID>
struct error_recovery_strategy {

    using result_type = std::tuple<bool, x3::error_handler_result>;

    template <typename It, typename Ctx>
    result_type operator()(It& first, It last, [[maybe_unused]] Ctx const& ctx) const {
        return error_recovery_common_strategy::call(first, last,
            error_recovery_strategy_aux<RuleID>());
    }
};

} // namespace detail

///
/// Syntactic sugar for specialized parser error recovery strategies.
///
/// It's using error strategy customization points, for concept
/// - https://godbolt.org/z/xrGqx557E
/// - https://godbolt.org/z/65rrn5qKx (alternative (constexpr) approach)
///
template <typename RuleID, typename It, typename Ctx>
auto error_recovery(It& first, It last, Ctx const& ctx) {
    return detail::error_recovery_strategy<RuleID>{}(first, last, ctx);
}

static unsigned error_count = 0;

///
/// Customizable parser error handler to use different error recovery strategies.
///
/// The general concept of taggable error handler is based on
/// [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
/// and is using several auxillary specialized classes to try to recover from parser
/// errors, @see error_recovery.
///
/// Reference: [Slack](https://cpplang.slack.com/archives/C27KZLB0X/p1662738072331189):
/// - @see https://godbolt.org/z/4qEfrP6GK (strong simplified, but works)
/// - @see https://godbolt.org/z/W8GvrY1qv (Sehe's reduction)
///
/// FIXME doesn't recover as intended, strategy map needs more effort, see also
/// [Slack](https://cpplang.slack.com/archives/C27KZLB0X/p1662738072331189)
///
template <typename RuleID>
struct my_x3_error_handler {
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& e, Ctx const& ctx) const
    {
        using detail::verbose_error_handler;
        auto& os = std::cout;
        ++error_count;

        if constexpr(verbose_error_handler) {
            os << fmt::format("*** error handler <{}> (error #{}) ***\n",
                              id(), error_count);
        }

        // This `on_error` catches the `x3::expectation_failure` exceptions and the exceptions
        // from the numerical conversion. From the X3 design, only the exceptions of the type
        // `x3::expectation_failure` are passed on from e.g. [parse_rhs_main()](
        // https://github.com/boostorg/spirit/blob/master/include/boost/spirit/home/x3/nonterminal/detail/rule.hpp)
        // to the error handler. Therefore, own ones must be derived from them and dynamically
        // up-casted within the handler to be able to distinguish between them. In this way, more
        // descriptive error messages can be elaborated - the actual intend. Concept:
        // @see [Coliru](https://coliru.stacked-crooked.com/a/4163d57183f76f93)
        auto const error_message = [](auto const& e) {
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

        auto const& error_handler = x3::get<x3::error_handler_tag>(ctx);
        error_handler(e.where(), error_message(e));

        auto const [proceed, error_handler_result] = error_recovery<RuleID>(first, last, ctx);
        if(proceed) {
            if constexpr(verbose_error_handler) {
                // pre-skip, but it's cosmetic
                x3::skip_over(first, last, ctx);
                os << "+++ ignoring errors and continue with:\n|"
                   << detail::excerpt_sv(first, last) << " ...|\n";
            }
            return error_handler_result;
        }

        if constexpr(verbose_error_handler) {
            os << "+++ give up on errors\n";
        }
        return x3::error_handler_result::fail;
    }

protected:
    // not intended to be derived from
    ~my_x3_error_handler() = default;

private:
    /// RuleID name string, in the same format as map key entries in error recovery
    /// strategy map, @see error_recovery_strategy_map
    static std::string id() {
        return std::type_index(typeid(RuleID)).name();
    }

};

}  // namespace namespace parser
