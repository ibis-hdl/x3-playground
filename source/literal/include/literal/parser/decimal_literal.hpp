//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parser/char_parser.hpp>
#include <literal/convert.hpp>

#include <boost/spirit/home/x3.hpp>

#include <cmath>
#include <limits>
#include <boost/safe_numerics/safe_integer.hpp>

#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>

#include <iostream>
#include <iomanip>

namespace parser {

namespace x3 = boost::spirit::x3;

namespace detail {

using char_parser::dec_digits;
using x3::char_;

// clang-format off
auto const exponent = [](auto&& signs) {
    using CharT = decltype(signs);
    return x3::rule<struct exponent_class, std::string>{ "exponent" } = x3::as_parser(
        x3::omit[ char_("Ee") ]
        >> x3::raw[ x3::lexeme [
             -char_(std::forward<CharT>(signs)) >> dec_digits
        ]]
    );
};
// clang-format on

// FixMe: Limit digits

// clang-format off
auto const signed_exp = x3::rule<struct _, std::string>{ "real exponent" } =
    exponent("-+");

auto const unsigned_exp = x3::rule<struct _, std::string>{ "integer exponent" } =
    exponent('+');

auto const real = x3::rule<struct _, ast::real_type>{ "literal real" } =
    dec_digits >> '.' >> x3::expect[dec_digits] >> -signed_exp;

auto const integer = x3::rule<struct _, ast::integer_type>{ "literal integer" } =
    dec_digits >> !x3::lit('#') >> -unsigned_exp;

// clang-format on

}  // namespace detail

namespace util {

// [cppreference.com std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
// helper type for the visitor
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename Expr>
std::string inspect_expr(Expr const&)
{
    return boost::core::demangle(typeid(Expr).name());
}

}  // namespace util

// BNF: decimal_literal := integer [ . integer ] [ exponent ]
struct decimal_literal : x3::parser<decimal_literal> {
    using attribute_type = ast::decimal_literal;

    template <typename IteratorT, typename ContextT, typename RContextT>
    bool parse(IteratorT& first, IteratorT const& last, [[maybe_unused]] ContextT const& ctx,
               [[maybe_unused]] RContextT const& rctx, attribute_type& attribute) const
    {
        using detail::integer;
        using detail::real;

        skip_over(first, last, ctx);
        auto const begin = first;

        auto const grammar = (real | integer) >> x3::attr(10U);

        auto const parse_ok = x3::parse(first, last, grammar, attribute);

        if (!parse_ok) {
            first = begin;  // rewind iter
            return false;
        }

        // clang-format off
        auto const convert = [this](attribute_type::num_type& var) {
            boost::apply_visitor(
                util::overloaded{
                    [this](ast::real_type& r) {
                        using target_type = ast::real_type::value_type;
                        r.value = decimal_literal::convert<target_type>(r, 10U);
                    },
                    [this](ast::integer_type& i) {
                        using target_type = ast::integer_type::value_type;
                        i.value = decimal_literal::convert<target_type>(i, 10U);
                    }
                }, var);
            // clang-format on
        };

        convert(attribute.num);

        return true;
    }

private:
    template <typename TargetT>
    std::optional<TargetT> convert(ast::real_type const& real, unsigned base = 10U) const
    {
        static_assert(std::is_floating_point_v<TargetT>, "TargetT must be of floating point type");

        // Concept, @see [godbolt.org](https://godbolt.org/z/KroM7oxc5)
        // @note: not relevant since std::string is returned, but generally take care on stack-use-after-scope, 
        // see [godbolt](https://godbolt.org/z/vr39h7Y8d)
        auto const as_real_string = [](ast::real_type const& real) 
        
            using namespace std::literals::string_view_literals;
            namespace views = ranges::views;

            auto const as_pruned_string = [](auto const& literal) {
                using namespace ranges;
                auto const pred = [](char chr) { return chr != '_'; };
                return ranges::to<std::string>(literal | views::join | views::filter(pred));
            };

            auto const as_sv = [](std::string const& str) { return std::string_view(str); };

            if (real.exponent.empty()) {
                auto const literal = { // --
                    as_sv(real.integer), "."sv, as_sv(real.fractional) };
                return as_pruned_string(literal);
            }
            auto const literal = { // --
                as_sv(real.integer), "."sv, as_sv(real.fractional), "e"sv, as_sv(real.exponent) };
            return as_pruned_string(literal);
        };

        std::string const real_string = as_real_string(real);

        std::error_code ec{};
        auto const real_result = convert::detail::as_real<TargetT>(real_string, base, ec);

        if (ec) {
            std::cerr << "decimal_literal error: " << ec.message() << " "
                      << std::quoted(real_string) << '\n';
            return {};
        }

        return real_result;
    }

    template <typename TargetT>
    std::optional<TargetT> convert(ast::integer_type const& integer, unsigned base = 10U) const
    {
        namespace views = ranges::views;

        static_assert(std::is_integral_v<TargetT> && std::is_unsigned_v<TargetT>,
                      "TargetT must be of unsigned integral type");

        std::string const int_string = convert::detail::prune(integer.integer);

        std::error_code ec{};
        auto const int_result = convert::detail::as_unsigned<TargetT>(int_string, base, ec);

        if (ec) {
            std::cerr << "decimal_literal error on integer: " << ec.message() << " "
                      << std::quoted(int_string) << '\n';
            return {};
        }

        if (integer.exponent.empty()) {
            // nothings to do
            return int_result;
        }

        // @note [from_chars](https://en.cppreference.com/w/cpp/utility/from_chars):
        // "the plus sign is not recognized outside of the exponent" - for VHDL's integer exponent
        // this is allowed (just handling this)! Since it may occur only at first character position 
        // it's get handled immediately. This avoids complexer (time consuming) range filter.
        auto exp_sv = std::string_view(integer.exponent);
        exp_sv.remove_prefix(std::min(exp_sv.find_first_not_of("+", 0, 1), exp_sv.size()));

        std::string const exp_string = convert::detail::prune(exp_sv);
        TargetT exp_index = convert::detail::as_unsigned<TargetT>(exp_string, base, ec);

        if (ec) {
            std::cerr << "decimal_literal error on exponent: " << ec.message() << " "
                      << std::quoted(exp_string) << '\n';
            return {};
        }

        // @note MSVC doesn't allow constexpr of log10, so an alternative way has been chosen, 
        // see [constexpr exp, log, pow](
        //  https://stackoverflow.com/questions/50477974/constexpr-exp-log-pow)
        static auto constexpr exp10_scale = convert::detail::exp10_scale<TargetT>;

        if (!(exp_index < exp10_scale.max_index())) {
            std::cerr << "decimal_literal error: exponent overflow\n";
        }

        try {
            // scale the integer with exponent, use [Safe Numerics](
            //  https://www.boost.org/doc/libs/develop/libs/safe_numerics/doc/html/index.html)
            // to guarantee to yield a correct mathematical result
            using namespace boost::safe_numerics;
            safe<TargetT> const i = int_result;
            safe<TargetT> const e = exp10_scale[exp_index];
            safe<TargetT> const result = i * e;
            return static_cast<TargetT>(result);
        }
        catch (std::exception const& e) {
            std::cerr << "decimal_literal error:" << e.what() << std::endl;
        }
        return {};
    }

} const decimal_literal;

}  // namespace parser
