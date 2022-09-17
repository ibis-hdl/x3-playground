//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <literal/ast.hpp>
#include <literal/parse.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <string>
#include <vector>

namespace testsuite_data {

std::vector<std::string> const lexeme_failure = {
// bit string literal
R"(
    X := z"xxxx_yyyy";  // wrong base specifier
    X := b"2000_0001";  // wrong charset
    X := o"8000_0001";  // wrong charset
    X := x"G000_0001";  // wrong charset
)",
// decimal literal
R"(
    X := 1e-3;          // neg. exponent not allowed
    X := 42,42;         // wrong decimal separator
)",
// based literal
R"(
    X := 37#1_20#E1;    // invalid base specifier
)",
// string literal
//   no idea
// char literal
R"(
    X := '';            // empty char literal is *not* allowed
)",
// numeric/physical literal
R"(
    X := 10.7 8ns;      // wrong unit
)"}; // vector<...> lexeme_failure

} // namespace testsuite_data

BOOST_AUTO_TEST_SUITE(literal_parser_lexeme_failure)

BOOST_AUTO_TEST_CASE(basic_parser_lexeme_failure)
{
    using stream_type = boost::test_tools::output_test_stream;

    for(auto idx = 0; auto const& input : testsuite_data::lexeme_failure) {
        reset_error_counter();
        auto os = stream_type{};
        ast::literals literals;
        bool parse_ok = parse(input, literals, os);
        BOOST_TEST(parse_ok == true);
        std::cout << os.str() << std::string(80, '=') << '\n';
    }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
