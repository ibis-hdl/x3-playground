//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/spirit/home/x3.hpp>

namespace parser::x3e { // Spirit x3 extended

namespace x3 = boost::spirit::x3;

namespace detail {

// [Boost spirit x3 - lazy parser with compile time known parsers, referring to a previously matched value](
//  https://stackoverflow.com/questions/72833517/boost-spirit-x3-lazy-parser-with-compile-time-known-parsers-referring-to-a-pr)

template <typename...>
struct Tag {
};

template <typename AttributeT, typename ParserT>
auto as(ParserT p, char const* name = "as")
{
    return x3::rule<Tag<AttributeT, ParserT>, AttributeT>{ name } = x3::as_parser(p);
}

template <typename Tag>
struct set_lazy_type {
    template <typename ParserT>
    auto operator[](ParserT p) const
    {
        auto action = [](auto& ctx) {  // set rhs parser
            x3::get<Tag>(ctx) = x3::_attr(ctx);
        };
        return p[action];
    }
};

template <typename Tag>
struct do_lazy_type : x3::parser<do_lazy_type<Tag>> {
    using attribute_type = typename Tag::attribute_type;  // TODO FIXME?

    template <typename It, typename Ctx, typename RCtx, typename Attr>
    bool parse(It& first, It last, Ctx& ctx, RCtx& rctx, Attr& attr) const
    {
        auto& subject = x3::get<Tag>(ctx);

        It saved = first;
        x3::skip_over(first, last, ctx);
        // clang-format off
        if(x3::as_parser(subject).parse(
               first,
               last,
               std::forward<Ctx>(ctx),
               std::forward<RCtx>(rctx),
               attr))
        {
            return true;
        }
        else {
            first = saved;
            return false;
        }
        // clang-format on
    }
};

}  // namespace detail

template <typename T>
static const detail::set_lazy_type<T> set_lazy{};
template <typename T>
static const detail::do_lazy_type<T> do_lazy{};

} // namespace parser::x3e
