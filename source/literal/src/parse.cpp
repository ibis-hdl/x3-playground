#include <literal/parse.hpp>
#include <literal/parser/literal.hpp>

#include <fmt/format.h>

#include <iostream>

bool parse(std::string const& input, ast::literals& literals, std::ostream& os) {

    try {
        auto iter = input.begin();
        auto const end = input.end();
        using error_handler_type = x3::error_handler<decltype(input.begin())>;
        error_handler_type error_handler(input.begin(), end, os, "input");

        auto const grammar = x3::with<x3::error_handler_tag>(error_handler)[
            parser::grammar >> -x3::eoi
        ];

        bool parse_ok = x3::parse(iter, end, grammar, literals);

        os << fmt::format("parse success: {}, {} error(s)\n", parse_ok, parser::error_count);
#if 0
        if(!literals.empty()) {
            os << "numeric literals:\n";
            for (auto const& lit : literals) {
                os << " - " << lit << '\n';
            }
        }
#endif
        return parse_ok;
    }
    catch (std::exception const& e) {
        os << fmt::format("caught in parse() '{}'\n", e.what());
        return false;
    }
    catch (...) {
        os << "caught in parse() 'Unexpected exception'\n";
        return false;
    }
}


void reset_error_counter() {
    parser::error_count = 0;
}
