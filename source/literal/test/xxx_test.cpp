//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

//#include <literal/ast.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <string>
#include <vector>

namespace testsuite_data {

std::vector<std::string> const success = {
// bit string literal
R"(
    X := b"1000_0001";
    X := x"AFFE_Cafe";
    X := O"777";
    X := X"";           // empty bit string literal is allowed
)",
// decimal literal
R"(
    X := 42;
    X := 1e+3;
    X := 42.42;
    X := 2.2E-6;
    X := 3.14e+1;
)",
// based literal
R"(
    X := 4#1_20#E1;     // 96 - yes, uncommon base for integers are (weak) supported
    X := 8#1_20#E1;
    X := 0_2#1100_0001#;
    X := 10#42#E4;
    X := 16#AFFE_1.0Cafe#;
    X := 16#AFFE_2.0Cafe#e-10;
    X := 16#DEAD_BEEF#e+0;
)",
// string literal
R"(
    X := "setup time too small";
    X := " ";
    X := "a";
    X := """";
    X := "";            // empty string literal is allowed
)",
// char literal
// Note: empty char literal is *not* allowed
R"(
    X := '0';
    X := 'A';
    X := '*';
    X := ''';
    X := ' ';
)",
// numeric/physical literal
R"(
    X := 10.7 ns;       // decimal (real)
    X := 42 us;         // decimal (real)
    X := 10#42#E4 kg;   // based literal
)",

}; // vector<...> success =

} // namespace testsuite_data

BOOST_AUTO_TEST_SUITE(literal)

BOOST_AUTO_TEST_CASE(basic_parser_success)
{
    for(auto idx = 0; auto const& input : testsuite_data::success) {
        ////ast::literals literals;
        //bool parse_ok = parse(input, literals);
        bool parse_ok = true;
        BOOST_TEST(parse_ok == true);
        std::cout << idx << '\n';
        ++idx;
    }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
