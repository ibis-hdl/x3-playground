//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once


namespace convert {

namespace leaf = boost::leaf;

template<typename ParserT, typename IteratorT>
static inline bool leaf_error_handlers2(ParserT const& self, IteratorT begin, auto&& convert_action) 
{
    //static_assert(std::is_base_of_v<x3::parser<ParserT>, ParserT>, "nix da");

    leaf::try_catch(
        [&] {
            convert_action();
            return true;
        },
#if 0
        [&](std::error_code const& ec, leaf::e_api_function const& api_fcn,
            leaf::e_position_iterator<IteratorT> const& e_iter) {
            // LEAF - from_chars()
            std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn.value,
                                        ec.message());
            boost::throw_exception(convert::numeric_failure<IteratorT>(
                // ex.where() - ex.which() - ex.what()
                e_iter, x3::what(self), ec.message()));
            return false;
        },
        [&](std::error_code const& ec, leaf::e_api_function const& api_fcn,
            leaf::e_fp_exception fp_exception) {
            // LEAF - safe_{mul,add}<RealT>
            std::cerr << fmt::format("LEAF: Error in api function '{}': {} ({})\n",
                                        api_fcn.value, ec.message(), fp_exception.as_string());
            boost::throw_exception(convert::numeric_failure<IteratorT>(
                // ex.where() - ex.which() - ex.what()
                begin, x3::what(self), ec.message()));
            return false;
        },
        [&](std::error_code const& ec, leaf::e_api_function const& api_fcn) {
            // LEAF - safe_{mu,add}<IntT>
            std::cerr << fmt::format("LEAF: Error in api function '{}': {}\n", api_fcn.value,
                                        ec.message());
            boost::throw_exception(convert::numeric_failure<IteratorT>(
                // ex.where() - ex.which() - ex.what()
                begin, x3::what(self), ec.message()));
            return false;
        },
        [&](std::error_code const& ec) {
            // LEAF
            std::cerr << fmt::format("LEAF: Error: '{}'\n", ec.message());
            boost::throw_exception(convert::numeric_failure<IteratorT>(
                // ex.where() - ex.which() - ex.what()
                begin, x3::what(self), ec.message()));
            return false;
        },
#endif
        [&](std::exception const& e) {
            std::cerr << fmt::format("LEAF: Caught exception: '{}'\n", e.what());
            boost::throw_exception(convert::numeric_failure<IteratorT>(
                // ex.where() - ex.which() - ex.what()
                begin, x3::what(self), e.what()));
            return false;
        }
    );
    return false;
}



// TODO tuple of LEAF error handlers to be used to capture errors thrown by conversion
// functions below.
// still partially used; but doesn't work finally since the out capture scope is required
// to handle the error with these informations. Full featured amd working version can be seen at
// bit_string_literal_parser
static inline auto leaf_error_handlers = std::make_tuple(

    [](std::error_code const& ec, leaf::e_api_function const& api_fcn
       // TODO templated argument / tuple
       /* leaf::e_position_iterator<IteratorT> const& e_iter */
       ) {
        std::cerr << fmt::format("Error in {} call: {}\n", api_fcn.value, ec.message());
        return false;
    },
    [](std::error_code const& ec) {
        std::cerr << fmt::format("Error: {}\n", ec.message());
        return false;
    },
    [](std::exception const& e) {
        std::cerr << fmt::format("Caught exception: '{}'\n", e.what());
        return false;
    },
    [](leaf::diagnostic_info const& info) {
        std::cerr << "Unrecognized error detected, cryptic diagnostic "
                     "information follows.\n"
                  << info;
        return false;
    });

}  // namespace convert
