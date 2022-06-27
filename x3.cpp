#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/struct.hpp>
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace ast {
    struct literal {
        std::uint32_t   base;
        std::uint32_t   integer;
        std::uint32_t   exponent;
    };
    using literals = std::vector<literal>;
    
    std::ostream& operator<<(std::ostream& os, ast::literal const& l) {
        os << fmt::format("{0}#{1}#{2}", l.base, l.integer, l.exponent) ;
        return os;
    }
} // namespace ast

BOOST_FUSION_ADAPT_STRUCT(ast::literal, base, integer, exponent)

namespace {

struct literal_base_type : x3::parser<literal_base_type> {

    using attribute_type = std::uint32_t;

    template<typename IteratorT, typename ContextT, typename RContextT, typename AttributeT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
        [[maybe_unused]] RContextT const& rctx, AttributeT& base_attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;
        // ...
        bool const parse_ok = x3::parse(first, last, x3::uint_, base_attribute);

        if (!parse_ok || !supported_base(base_attribute)) {
            first = begin;
            return false;
        }
        return true;
    }

    bool supported_base(attribute_type base) const {
        if (base == 2 || base == 8 || base == 10 || base == 16) {
            return true;
        }
        return false;
    }
};

x3::rule<struct literal_class, ast::literal> const literal{ "literal parser" };
x3::rule<struct grammar_class, ast::literals> const grammar{ "grammar" };

auto const literal_def = x3::lexeme[
       x3::expect[x3::uint_] // base
    >> '#' 
    >> x3::uint_ // integer
    >> '#' 
    >> x3::uint_ // exponent
];

auto const comment = "//" >> *(x3::char_ - x3::eol) >> x3::eol;
auto const grammar_def = x3::skip(x3::space | comment)[
    *(literal >> x3::expect[';'])
];

BOOST_SPIRIT_DEFINE(literal, grammar)

} // namespace

int main() {
    std::string const input = R"(
    2#011#;
    8#120#1;
    10#42#0;
    16#AFFE#CAFE#10;
    // failure
    3#011#;      // base not supported
    2#120#1;     // wrong char set
    10#42#666;   // exp can't fit double (e308)
    8#1#1        // forgott ';' - otherwise ok
    // ok, just to test success after
    10#42#42;
)";
    ast::literals literals;
    bool parse_ok = x3::parse(input.begin(), input.end(), grammar, literals);
    std::cout << fmt::format("parse ok is '{0}', numeric literals:\n", parse_ok);
    for(auto const& lit: literals) {
        std::cout << lit << '\n';
    }
}
