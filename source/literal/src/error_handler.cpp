//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/spirit/home/x3.hpp>

#include <literal/parser/error_handler.hpp>
#include <literal/parser/parser_id.hpp>

#include <map>
#include <typeindex>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace x3 = boost::spirit::x3;

template <> struct fmt::formatter<x3::error_handler_result>;

namespace parser {

namespace detail {

error_recovery_common_strategy::result_type
error_recovery_common_strategy::call(iterator_type& first, iterator_type last,
                                     error_recovery_strategy_map::lookup_result const& recovery_aux)
{
    auto& os = std::cout;

    auto const symbol = recovery_aux.symbol;

    if constexpr(verbose_error_handler) {
        static auto const find_grammar = x3::skip(x3::space | parser::comment)[
            x3::lexeme[ +(x3::char_ - symbol) >> x3::char_(symbol) ]
        ];
        os << fmt::format("+++ recover in: |{} ...|\n", excerpt_sv(first, last));
        std::string skip_over_str;
        if(x3::parse(first, last, find_grammar, skip_over_str)) {
            os << fmt::format("+++ recover skip: |{} ...|\n", skip_over_str);
            return { true, recovery_aux.handler_result };
        }
    }
    else {
        static auto const find_grammar = x3::skip(x3::space | parser::comment)[
            +(x3::char_ - symbol) >> symbol
        ];
        if(x3::parse(first, last, find_grammar)) {
            return { true, recovery_aux.handler_result };
        }
    }

    // no way ...
    return { false, x3::error_handler_result::fail };
}

template <typename TagT>
struct make_typeid {
    static inline std::type_index const id = std::type_index(typeid(TagT));
};

template <typename TagT>
static auto constexpr type = make_typeid<TagT>{};

template <typename TagT>
static auto constexpr rule_id() { return detail::type<TagT>.id; };

// concept https://godbolt.org/z/b5vdM5TKo

error_recovery_strategy_map::lookup_result error_recovery_strategy_map::lookup(std::type_index id)
{
    auto& os = std::cout;

    using x3::error_handler_result::accept;
    using x3::error_handler_result::retry;
    using x3::error_handler_result::rethrow;
    using x3::error_handler_result::fail;

    // on default (map's lookup failed), we accept the parser error and hope that the
    // 'smart' and default implementation of error_recovery_strategy class does the
    // job right here ...
    static lookup_result constexpr default_result = { ';', accept };

    // type_index 8 byte, enum x3::error_handler_result 4 byte
    static std::map<std::type_index, lookup_result> const rule_id_results({
#if 0
        { rule_id<based_literal_class>(),       { ';', accept } },
        { rule_id<decimal_literal_class>(),     { ';', accept } },
        { rule_id<bit_string_literal_class>(),  { ';', accept } },
        { rule_id<character_literal_class>(),   { ';', accept } },
        { rule_id<string_literal_class>(),      { ';', accept } },
#endif
        { rule_id<literal_rule_class>(),        { ';', retry  } },
        { rule_id<grammar_class>(),             { ';', accept } },
        // ...
    });

    auto const search = rule_id_results.find(id);

    if (search != rule_id_results.end()) {
        if constexpr(verbose_error_handler) {
            fmt::print(os, "+++ recovery RuleID '{}' -> {}\n",
                    search->first.name(), search->second.handler_result);
        }
        return search->second;
    }

    if constexpr(verbose_error_handler) {
        fmt::print(os, "+++ recovery RuleID '{}' -> {} (default)\n",
                   id.name(), default_result.handler_result);
    }

    return default_result;
}

} // namespace detail

} // namespace parser

//
// {fmt} lib formatter
//

template <> struct fmt::formatter<x3::error_handler_result> {

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(x3::error_handler_result result, FormatContext& ctx) const -> decltype(ctx.out()) {

    switch(result) {
        case x3::error_handler_result::accept:
            return fmt::format_to(ctx.out(), "{}", "accept");
        case x3::error_handler_result::retry:
            return fmt::format_to(ctx.out(), "{}", "retry");
        case x3::error_handler_result::rethrow:
            return fmt::format_to(ctx.out(), "{}", "rethrow");
        case x3::error_handler_result::fail:
            return fmt::format_to(ctx.out(), "{}", "fail");
    }
    return ctx.out();
  }
};
