//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once


namespace convert {

namespace leaf = boost::leaf;

// TODO tuple of LEAF error handlers to be used to capture errors thrown by conversion
// functions below.
// still partially used; but doesn't work finally since the out capture scope is required
// to handle the error with these informations. Full featured amd working version can be seen at
// bit_string_literal_parser
static inline auto const leaf_error_handlers = std::make_tuple(

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
