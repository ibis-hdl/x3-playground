#include <literal/ast.hpp>

#include <fmt/format.h>

#include <iostream>
#include <iomanip>

namespace ast {

std::ostream& operator<<(std::ostream& os, ast::real_type const& r)
{
    os << fmt::format("#{}.{}#e{}", r.integer, r.fractional, r.exponent.value_or(1));
    return os;
}

std::ostream& operator<<(std::ostream& os, ast::integer_type const& i)
{
    os << fmt::format("#{}#e{}", i.integer, i.exponent.value_or(1));
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
    os << fmt::format(R"({}"{}")", l.base, l.integer);
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
