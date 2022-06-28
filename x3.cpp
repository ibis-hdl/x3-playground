//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/optional.hpp>
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace ast {

template <typename... Types> using variant = x3::variant<Types...>;
template <typename T>        using optional = boost::optional<T>;

struct based_literal {
    using real_type = struct {
        std::string             integer;    // base: 2,8,10,16
        std::string             fractional; // base: 2,8,10,16
        optional<std::int32_t>  exponent;
    };
    using integer_type = struct {
        std::string             integer;  // base: 2,8,10,16
        optional<std::uint32_t> exponent; // positive only!
    };
    std::uint32_t                       base;
    variant<real_type, integer_type>    num;
    // std::variant<double, unsigned> const value() // lazy numeric conversion
};
using based_literals = std::vector<based_literal>;
using decimal_literal = variant<std::int64_t, double>;
using decimal_literals = std::vector<decimal_literal>;
using literal = variant<based_literal, decimal_literal>;
using literals = std::vector<literal>;

std::ostream& operator<<(std::ostream& os, ast::based_literal::real_type const& r) {
    os << fmt::format("#{0}.{1}#{2}", r.integer, r.fractional, r.exponent.value_or(1));
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::based_literal::integer_type const& i) {
    os << fmt::format("#{0}#{1}", i.integer, i.exponent.value_or(1));
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::based_literal const& l) {
    struct v {
        void operator()(based_literal::real_type const& r) const { os << r; }
        void operator()(based_literal::integer_type const& i) const { os << i; }
        v(std::ostream& os_) : os{ os_ }{}
        std::ostream& os;
    } const v(os);
    os << l.base;
    boost::apply_visitor(v, l.num);
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& l) {
    struct v {
        std::ostream& os;
        v(std::ostream& os_) : os{ os_ }{}
        void operator()(std::int64_t i) const { os << i; }
        void operator()(double d) const { os << d; }
    } const v(os);
    boost::apply_visitor(v, l);
    return os;
}

} // namespace ast

BOOST_FUSION_ADAPT_STRUCT(ast::based_literal::real_type, integer, fractional, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::based_literal::integer_type, integer, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::based_literal, base, num)

namespace {

x3::rule<struct based_literal_class, ast::based_literal> const based_literal{ "based literal" };
x3::rule<struct grammar_class, ast::based_literals> const grammar{ "grammar" };

using x3::char_;
using x3::uint_;
using x3::int_;
using x3::lexeme;
using x3::lit;
using x3::omit;
using x3::raw;

#if 0
template<typename T>
auto as = [](auto p, char const* name = typeid(decltype(p)).name()) {
    return x3::rule<struct _, T>{ name } = x3::as_parser(p);
};
#endif

auto const comment = "//" >> *(char_ - x3::eol) >> x3::eol;

auto const base = x3::rule<struct _, std::uint32_t>{ "literal base" } =
    uint_;

// https://coliru.stacked-crooked.com/a/3ec0d3b795b5ce62
auto const signed_exp = x3::rule<struct _, std::int32_t>{ "literal real exponent" } =
    omit[ char_("Ee") ] >> int_;

// https://coliru.stacked-crooked.com/a/7acce6f83686ccb4
auto const unsigned_exp = x3::rule<struct _, std::uint32_t>{ "literal integer exponent" } =
    omit[ char_("Ee") >> -lit('+') ] >> uint_;

// https://coliru.stacked-crooked.com/a/7f7f91bbe6c1873e
auto const based_digit = x3::rule<struct _, std::string>{ "literal digit" } =
    raw[ +x3::xdigit ];

// https://coliru.stacked-crooked.com/a/f58b117dcc8dd594
auto const real = x3::rule<struct _, ast::based_literal::real_type>{ "literal real" } =
    /* lexeme */ based_digit >> '.' >> based_digit >> '#' >> -signed_exp;

auto const integer = x3::rule<struct _, ast::based_literal::integer_type>{ "literal integer" } =
    /* lexeme */ based_digit >> '#' >> -unsigned_exp;

// base # based_integer [ . based_integer ] # [ exponent ]
auto const based_literal_def = x3::lexeme[ base >> '#' >> (real | integer) ];
auto const grammar_def = x3::skip(x3::space | comment)[ *(based_literal >> ';') ];

BOOST_SPIRIT_DEFINE(based_literal, grammar)

} // namespace

int main()
{
    std::string const input = R"( // todo ...
    02#110#;
    8#120#E1;
    10#42#E42;
    16#AFFE.0CAFE#e-10;
    // failure
    //3#011#;      // base not supported
    //2#120#1;     // wrong char set
    //10#42#666;   // exp can't fit double (e308)
    //8#1#1        // forgot ';' - otherwise ok
    //8#1#         // also forgot ';' - otherwise ok
    // ok, just to test success after
    10#42#42;
)";

    ast::based_literals literals;
    bool parse_ok = x3::parse(input.begin(), input.end(), grammar, literals);
    std::cout << fmt::format("parse ok is '{0}', numeric literals:\n", parse_ok);
    for(auto const& lit: literals) {
        std::cout << lit << '\n';
    }
}

// https://stackoverflow.com/search?q=user%3A85371+x3+parser_base
// https://stackoverflow.com/questions/49722452/combining-rules-at-runtime-and-returning-rules/49722855#49722855

/*
Hi, I try to reduce my spirit x3 problem and failed on the basics ... https://coliru.stacked-crooked.com/a/7e5e6c47ffce13cd compiles, but the result isn't what I'm expected. It seems that the skipper doesn't skip white spaces. How can I get it parsing as intended?
*/

#if 0 // save snippets / store

// see [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
template <typename RuleID>
struct error_handler {
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& x, Ctx const& ctx) const {
        std::cerr << fmt::format("In '{0}' expected {1}\n", std::string(first, last), x.which());
        first = x.where();
        return x3::error_handler_result::accept;
    }
};

template <typename T = x3::unused_type, typename RuleID>
auto as(auto p, char const* name = typeid(decltype(p)).name()) {
    using tag = error_handler<RuleID>;
    return x3::rule<tag, T>{ name } = p;
}

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
#endif
