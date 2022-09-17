#include <literal/ast.hpp>
#include <literal/util/overloaded.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <iostream>
#include <iomanip>

template <typename T>
struct fmt::formatter<std::optional<T>> : fmt::formatter<T> {
    template <typename FormatContext>
    auto format(const std::optional<T>& opt, FormatContext& ctx)
    {
        if (opt) {
            fmt::formatter<T>::format(*opt, ctx);
            return ctx.out();
        }
        return fmt::format_to(ctx.out(), "N/A");
    }
};

namespace ast {

bool constexpr print_node_name = true;

std::ostream& operator<<(std::ostream& os, ast::real_type const& real)
{
    fmt::print(os, "{}#{}.{}#", real.base, real.integer, real.fractional);
    if (!real.exponent.empty()) {
        fmt::print(os, "e{}", real.exponent);
    }
    if (real.value) {
        fmt::print(os, " ({}r)", real.value.value());
    }

    if constexpr(print_node_name) {
        fmt::print(os, " -> (real_type,");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::integer_type const& int_)
{
    fmt::print(os, "{}#{}#", int_.base, int_.integer);
    if (!int_.exponent.empty()) {
        fmt::print(os, "e{}", int_.exponent);
    }
    if (int_.value) {
        fmt::print(os, " ({}i)", int_.value.value());
    }

    if constexpr(print_node_name) {
        fmt::print(os, " -> (integer_type,");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::based_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::real_type const& real) { fmt::print(os, "{}", real); },
        [&](ast::integer_type const& int_) { fmt::print(os, "{}", int_); }
    }, literal.num);

    if constexpr(print_node_name) {
        fmt::print(os, " based_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::real_type const& real) { fmt::print(os, "{}", real); },
        [&](ast::integer_type const& int_) { fmt::print(os, "{}", int_); }
    }, literal.num);

    if constexpr(print_node_name) {
        fmt::print(os, " decimal_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::abstract_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::based_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::decimal_literal lit) { fmt::print(os, "{}", lit); }
    },  literal);

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::bit_string_literal const& literal)
{
    fmt::print(os, R"({}"{}")", literal.base, literal.literal);
    if (literal.value) {
        fmt::print(os, " ({}d)", literal.value.value());
    }

    if constexpr(print_node_name) {
        fmt::print(os, " -> (bit_string_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::identifier const& ident)
{
    fmt::print(os, "{}", ident.name);

    if constexpr(print_node_name) {
        // Ternary operator for keyword "hack" of NULL
        fmt::print(os, " -> (identifier{}", ident.name.starts_with("kw:") ? ")" : ", ");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::physical_literal const& literal)
{
    fmt::print(os, "{} [{}]", literal.literal, literal.unit_name);

    if constexpr(print_node_name) {
        fmt::print(os, " -> (physical_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::numeric_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::abstract_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::physical_literal lit) { fmt::print(os, "{}", lit); }
    }, literal);

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::character_literal const& literal)
{
    fmt::print(os, R"("{}")", literal.literal);

    if constexpr(print_node_name) {
        fmt::print(os, " -> (character_literal, ");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::enumeration_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::identifier ident) { fmt::print(os, "{}", ident); },
        [&](ast::character_literal lit) { fmt::print(os, "{}", lit); }
    }, literal);

    if constexpr(print_node_name) {
        fmt::print(os, "enumeration_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::string_literal const& literal)
{
    fmt::print(os, R"('{}')", literal.literal);

    if constexpr(print_node_name) {
        fmt::print(os, " -> (string_literal)");
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::literal const& literal)
{
    boost::apply_visitor(util::overloaded {
#if 1
        [&](auto&& lit) { fmt::print(os, "{}", lit); },
        [&]([[maybe_unused]] std::monostate) { fmt::print(os, "monostate"); }
#else
        [&](ast::numeric_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::enumeration_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::string_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::bit_string_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::identifier lit) { fmt::print(os, "{}", lit); },
        [&]([[maybe_unused]] std::monostate) { fmt::print(os, "monostate"); }
#endif
    }, literal);

    return os;
}

}  // namespace ast
