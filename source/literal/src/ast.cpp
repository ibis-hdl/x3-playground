#include <literal/ast.hpp>

#include <fmt/format.h>

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

std::ostream& operator<<(std::ostream& os, ast::real_type const& r)
{
    os << fmt::format("#{}.{}#", r.integer, r.fractional);
    if (!r.exponent.empty()) {
        os << fmt::format("e{}", r.exponent);
    }
    if (r.value) {
        os << fmt::format(" ({}r)", r.value.value());
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::integer_type const& i)
{
    os << fmt::format("#{}#", i.integer);
    if (!i.exponent.empty()) {
        os << fmt::format("e{}", i.exponent);
    }
    if (i.value) {
        os << fmt::format(" ({}i)", i.value.value());
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::based_literal const& l)
{
    struct v {
        void operator()(ast::real_type const& r) const { os << r; }
        void operator()(ast::integer_type const& i) const { os << i; }
        std::ostream& os;
        v(std::ostream& os_)
            : os{ os_ }
        {
        }
    } const v(os);
    os << l.base;
    boost::apply_visitor(v, l.num);
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& l)
{
    struct v {
        void operator()(ast::real_type const& r) const { os << r; }
        void operator()(ast::integer_type const& i) const { os << i; }
        std::ostream& os;
        v(std::ostream& os_)
            : os{ os_ }
        {
        }
    } const v(os);
    os << l.base;
    boost::apply_visitor(v, l.num);

    return os;
}

std::ostream& operator<<(std::ostream& os, ast::abstract_literal const& l)
{
    struct v {
        void operator()(ast::based_literal b) const { os << b; }
        void operator()(ast::decimal_literal d) const { os << d; }
        std::ostream& os;
        v(std::ostream& os_)
            : os{ os_ }
        {
        }
    } const v(os);
    boost::apply_visitor(v, l);
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::bit_string_literal const& l)
{
    os << fmt::format(R"({}"{}")", l.base, l.literal);
    if (l.value) {
        os << fmt::format(" ({}d)", l.value.value());
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::literal const& l)
{
    struct v {
        void operator()(ast::abstract_literal a) const { os << a; }
        void operator()(ast::bit_string_literal b) const { os << b; }
        std::ostream& os;
        v(std::ostream& os_)
            : os{ os_ }
        {
        }
    } const v(os);
    boost::apply_visitor(v, l);
    return os;
}

}  // namespace ast
