#include <literal/ast.hpp>
#include <literal/util/overloaded.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <iostream>
#include <iomanip>

namespace fmt {

template <typename T>
struct formatter<std::optional<T>> : fmt::formatter<T> {
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

}  // namespace fmt

namespace ast {

std::ostream& operator<<(std::ostream& os, ast::real_type const& real)
{
    fmt::print(os, "{}#{}.{}#", real.base, real.integer, real.fractional);
    if (!real.exponent.empty()) {
        fmt::print(os, "e{}", real.exponent);
    }
    if (real.value) {
        fmt::print(os, " ({}r)", real.value.value());
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

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::based_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::real_type const& real) { fmt::print(os, "{}", real); },
        [&](ast::integer_type const& int_) { fmt::print(os, "{}", int_); }
    }, literal.num);

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::real_type const& real) { fmt::print(os, "{}", real); },
        [&](ast::integer_type const& int_) { fmt::print(os, "{}", int_); }
    }, literal.num);

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

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::abstract_literal lit) { fmt::print(os, "{}", lit); },
        [&](ast::bit_string_literal lit) { fmt::print(os, "{}", lit); }
    }, literal);

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::identifier const& int_)
{
    fmt::print(os, "{}", int_.name);
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::physical_literal const& literal)
{
    fmt::print(os, "{} [{}]", literal.literal, literal.unit_name);
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
    fmt::print(os, "'{}'", literal.literal);
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::enumeration_literal const& literal)
{
    boost::apply_visitor(util::overloaded {
        [&](ast::identifier ident) { fmt::print(os, "{}", ident); },
        [&](ast::character_literal lit) { fmt::print(os, "{}", lit); }
    }, literal);

    return os;
}

}  // namespace ast
