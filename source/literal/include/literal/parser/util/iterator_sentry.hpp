//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <iostream>

namespace parser::util {

// concept work in progress: https://coliru.stacked-crooked.com/a/ac458d8acf787ccb

// Utility to recover the iterator in case of parsing or conversion failure. This is
// intended to be used with boost LEAF to simplify error handling.
template <typename IterT>
struct iterator_sentry {
    iterator_sentry(IterT iter, bool const& sentry)
        : iter_ref{ iter }
        , iter_bak{ iter }
        , ok{ sentry }
    {
    }

    ~iterator_sentry()
    {
        if (!ok) {
            std::cerr << "iterator_sentry: restore iterator\n";
            iter_ref = iter_bak;
        }
    }

    iterator_sentry() = delete;
    iterator_sentry(iterator_sentry&) = delete;
    iterator_sentry& operator=(iterator_sentry&) = delete;
    iterator_sentry(iterator_sentry&&) = delete;
    iterator_sentry& operator=(iterator_sentry&&) = delete;

private:
    IterT& iter_ref;
    IterT const iter_bak;
    bool const& ok;
};

}  // namespace parser::util