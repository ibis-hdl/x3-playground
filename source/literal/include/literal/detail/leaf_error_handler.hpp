//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <literal/convert_error.hpp>
#include <literal/detail/leaf_errors.hpp>

#include <boost/leaf.hpp>

#include <fmt/format.h>

namespace convert {

namespace leaf = boost::leaf;

///
/// Tuple of LEAF error handlers to be used to capture errors thrown by conversion
/// functions.
///
/// @see [Binding Error Handlers in a std::tuple](
///  https://www.boost.org/doc/libs/1_80_0/libs/leaf/doc/html/index.html#tutorial-binding_handlers)
///
template<typename IteratorT>
static inline auto const leaf_error_handlers = std::make_tuple(

    [](std::error_code const& ec, leaf::e_x3_parser_context<IteratorT>& parser_ctx,
        leaf::e_api_function const* api_fcn, leaf::e_fp_exception const* fp_exception,
        leaf::e_position_iterator<IteratorT> const* e_iter) {

        std::cerr << "LEAF handler #1 called.\n";
        // LEAF - e.g. safe_{mu,add}<IntT>
        if(api_fcn) {
            std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn->value, ec.message());
        }
        if(fp_exception) {
            std::cerr << fmt::format("LEAF: Error during FP operation '{}': {}\n", fp_exception->as_string(), ec.message());
        }
        parser_ctx.unroll();
        // The e_iter is set by from_chars API and points to erroneous position. If not known take the iterator
        // position from outer parser.
        IteratorT iter = (e_iter) ? e_iter->value : parser_ctx.iter();

        leaf::throw_exception( // --
            // FixMe This call results into ASAN error 'alloc_dealloc_mismatch' inside (371d2c2) by use with libc++, or
            // attempting free on address which was not malloc()-ed
            // which seems to be related to leaf::e_x3_parser_context
            convert::numeric_failure<IteratorT>(
                // notation x3::expectation_failure(where, which, what)
                iter, parser_ctx.which(), ec.message()
        ));
        return false;
    },
    [](std::exception const& e, leaf::e_x3_parser_context<IteratorT>& parser_ctx) {
        std::cerr << "LEAF handler #2 called.\n";
        parser_ctx.unroll();
        leaf::throw_exception( // --
            convert::numeric_failure<IteratorT>(
                // notation x3::expectation_failure(where, which, what)
                parser_ctx.iter(), parser_ctx.which(), e.what()
        ));
        return false;
    },
    [](leaf::error_info const& unmatched)
    {
        std::cerr << "LEAF: Unknown failure detected:\n" << unmatched;
        // TODO Here is no parser context available, so recovery and useful error handling isn't
        // possible anymore. This is considered as serious bug.
        abort();
        return false;
    });

}  // namespace convert
