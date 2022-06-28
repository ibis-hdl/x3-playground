// https://coliru.stacked-crooked.com/a/c4551ac14ca71175
// https://godbolt.org/z/fYExGrYzd
//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/optional.hpp>
#include <fmt/format.h>
#include <range/v3/view/filter.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <iomanip>

namespace x3 = boost::spirit::x3;

namespace ast {

template <typename... Types> using variant = x3::variant<Types...>;
template <typename T>        using optional = boost::optional<T>;

struct real_type : x3::position_tagged  {
    // boost::iterator_range<Iter> later on
    std::string             integer;    // base: 2,8,10,16
    std::string             fractional; // base: 2,8,10,16
    optional<std::int32_t>  exponent;
};
struct integer_type : x3::position_tagged  {
    std::string             integer;    // base: 2,8,10,16
    optional<std::uint32_t> exponent;   // positive only!
};
struct based_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    std::uint32_t               base;
    num_type                    num;
    // e.g. https://coliru.stacked-crooked.com/a/652a879e37b4ea37
    // std::variant<RealT, IntT> const value() // lazy numeric conversion
};
struct decimal_literal : x3::position_tagged {
    using num_type = variant<real_type, integer_type>;
    std::uint32_t               base = 10; // dummy
    num_type                    num;
    // std::variant<RealT, IntT> const value() // lazy numeric conversion
};
using literal = variant<based_literal, decimal_literal>; // Todo unify!
using literals = std::vector<literal>;

using decimal_literals = std::vector<decimal_literal>;

std::ostream& operator<<(std::ostream& os, ast::real_type const& r) {
    os << fmt::format("#{}.{}#e{}", r.integer, r.fractional, r.exponent.value_or(1));
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::integer_type const& i) {
    os << fmt::format("#{}#e{}", i.integer, i.exponent.value_or(1));
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::based_literal const& l) {
    struct v {
        void operator()(ast::real_type const& r) const { os << r; }
        void operator()(ast::integer_type const& i) const { os << i; }
        std::ostream& os;
        v(std::ostream& os_) : os{ os_ } {}
    } const v(os);
    os << l.base;
    boost::apply_visitor(v, l.num);
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::decimal_literal const& l) {
    struct v {
        void operator()(ast::real_type const& r) const { os << r; }
        void operator()(ast::integer_type const& i) const { os << i; }
        std::ostream& os;
        v(std::ostream& os_) : os{ os_ } {}
    } const v(os);
    os << l.base;
    boost::apply_visitor(v, l.num);
    return os;
}
std::ostream& operator<<(std::ostream& os, ast::literal const& l) {
    struct v {
        void operator()(ast::based_literal b) const { os << b; }
        void operator()(ast::decimal_literal d) const { os << d; }
        std::ostream& os;
        v(std::ostream& os_) : os{ os_ } {}
    } const v(os);
    boost::apply_visitor(v, l);
    return os;
}

} // namespace ast

BOOST_FUSION_ADAPT_STRUCT(ast::real_type, integer, fractional, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::integer_type, integer, exponent)
BOOST_FUSION_ADAPT_STRUCT(ast::based_literal, base, num)
BOOST_FUSION_ADAPT_STRUCT(ast::decimal_literal, base, num)

namespace {

// [Custom error on rule level? #657](https://github.com/boostorg/spirit/issues/657)
// for different error recovery strategies
//template <typename RuleID>
struct my_error_handler { // try to recover and continue to parse
    template <typename It, typename Ctx>
    auto on_error(It& first, It last, x3::expectation_failure<It> const& e, Ctx const& ctx) const {
        //std::cerr << "(" << typeid(RuleID).name() << " error handler fired) ";
        auto& error_handler = x3::get<x3::error_handler_tag>(ctx);
        std::string message = "Error! Expecting " + e.which() + " here:";
        error_handler(e.where(), message);
        // FixMe: always true: e.where() == first ; see https://github.com/boostorg/spirit/issues/726
        // our error resolution strategy
        auto const position = std::string_view(first, last).find_first_of(";");
        if (position != std::string_view::npos) {
            std::advance(first, position + 1); // move iter behind
            std::cout << fmt::format("Rest:\n---8<---\n{}\n--->8---\n", std::string_view(first, last));
        } else {
            first = last; // no way here :(
            return x3::error_handler_result::fail;
        }
        return x3::error_handler_result::accept;
    }
};

template<unsigned Base, typename AttributeT>
struct based_integer_parser : x3::parser<based_integer_parser<Base, AttributeT>> {

    static_assert(std::is_integral<AttributeT>::value, "Integral required.");

    using attribute_type = AttributeT;

    auto constexpr parser() const {
        if constexpr (std::is_unsigned<AttributeT>::value) {
            return x3::uint_parser<AttributeT, Base>{};
        }
        else {
            return x3::int_parser<AttributeT, Base>{};
        }
    }

    template<typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
        [[maybe_unused]] RContextT const& rctx, AttributeT& base_attribute) const
    {
        namespace views = ranges::views;
        skip_over(first, last, ctx);
        auto const begin = first;
        std::string_view const literal_sv{ first, last };
        auto view = literal_sv | views::filter([](char chr) { return chr != '_'; });
        bool const parse_ok = x3::parse(std::begin(view), std::end(view), parser(), base_attribute);

        if (!parse_ok) {
            first = begin; // rewind iter
            return false;
        }
        return true;
    }
};

template<unsigned Base = 10U, typename AttributeT = std::uint32_t>
struct literal_base_type : x3::parser<literal_base_type<Base, AttributeT>> {

    using attribute_type = AttributeT;

    template<typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
        [[maybe_unused]] RContextT const& rctx, AttributeT& base_attribute) const
    {
        skip_over(first, last, ctx);
        auto const begin = first;
        static auto const parser = based_integer_parser<Base, attribute_type>{};
        bool const parse_ok = x3::parse(first, last, parser, base_attribute);

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

// [How do I get which() to work correctly in boost spirit x3 expectation_failure?](
// https://stackoverflow.com/questions/71281614/how-do-i-get-which-to-work-correctly-in-boost-spirit-x3-expectation-failure)
template <typename T = x3::unused_type>
auto as(auto p, char const* name = typeid(decltype(p)).name()) {
    using tag = my_error_handler;
    return x3::rule<tag, T>{ name } = p;
}

template <typename T>
auto mandatory(auto p, char const* name = typeid(decltype(p)).name()) {
    return x3::expect[ as<T>(p, name) ];
}

using x3::char_;
using x3::lit;

auto const base_ = literal_base_type{};
auto const int_ =  based_integer_parser<10U, std::int32_t>{};
auto const uint_ = based_integer_parser<10U, std::uint32_t>{};

auto const comment = "//" >> *(char_ - x3::eol) >> x3::eol;

auto const base = x3::rule<struct _, std::uint32_t>{ "literal base" } =
    base_;
auto const signed_exp = x3::rule<struct _, std::int32_t>{ "literal real exponent" } =
    x3::omit[ char_("Ee") ] >> int_;
auto const unsigned_exp = x3::rule<struct _, std::uint32_t>{ "literal integer exponent" } =
    x3::omit[ char_("Ee") >> -lit('+') ] >> uint_;
auto const based_digit = x3::rule<struct _, std::string>{ "literal digit" } =
    x3::raw[ x3::xdigit >> *( -lit('_') >> x3::xdigit) ]; // FixMe: char set depends on base
auto const real = x3::rule<struct _, ast::real_type>{ "literal real" } =
    based_digit >> '.' >> x3::expect[ based_digit ] >> '#' >> -signed_exp;
auto const integer = x3::rule<struct _, ast::integer_type>{ "literal integer" } =
    based_digit >> '#' >> -unsigned_exp;

// BNF: based_literal := base # based_integer [ . based_integer ] # [ exponent ]
auto const based_literal = x3::rule<struct based_literal_class, ast::decimal_literal>{ "based literal" } =
    x3::lexeme[
           base >> '#' >> mandatory<ast::based_literal::num_type>( real | integer, "based literal real or integer type")
    ];

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
auto const decimal_literal = x3::rule<struct decimal_literal_class, ast::decimal_literal>{ "decimal literal" } =
    x3::attr(10U) >> (real | integer);

auto const grammar = x3::rule<struct grammar_class, ast::literals>{ "literal" } =
    x3::skip(x3::space | comment)[
        *((based_literal | decimal_literal) >> x3::expect[';'])
    ];

struct grammar_class : my_error_handler {};

} // namespace

int main()
{
    using namespace ast;

    std::string const input = R"(
    // based literal
    0_2#1100_0001#;
    8#1_20#E1;
    10#42#E42;
    16#AFFE_1.0CAFE#e-10;
    // decimal literal
    42;
    42.42;
    2.2E-6;
    1e+3;
    // failure test
    2##;          // - based literal real or integer type
    // failure
    3#011#;       // base not supported
    2#120#1;      // wrong char set for binary
    10#42#666;    // exp can't fit double (e308)
    8#1#e1        // forgot ';' - otherwise ok
    8#2#          // also forgot ';' - otherwise ok
    16#1.2#e;     // forgot exp num
    // ok, just to test success afterwards
    10#42.666#e4711;
)";

    try {
        using error_handler_type = x3::error_handler<decltype(input.begin())>;
        error_handler_type error_handler(input.begin(), input.end(), std::cerr);

        auto const grammar_ = x3::with<x3::error_handler_tag>(error_handler) [
            grammar >> x3::eoi
        ];

        ast::literals literals;
        bool parse_ok = x3::parse(input.begin(), input.end(), grammar_, literals);
        std::cout << fmt::format("parse ok is '{}', numeric literals:\n", parse_ok);
        for(auto const& lit: literals) {
            std::cout << lit << '\n';
        }
    } catch(std::exception const& e) {
        std::cerr << fmt::format("caught {}\n", e.what());
    } catch(...) {
        std::cerr << "caught unexpected exception\n";
    }
}

// https://stackoverflow.com/search?q=user%3A85371+x3+parser_base
// https://stackoverflow.com/questions/49722452/combining-rules-at-runtime-and-returning-rules/49722855#49722855
