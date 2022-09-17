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

std::vector<std::string> const success_input = {
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
)"}; // vector<...> success_input

std::string const os_expect =
R"(parse success: true, 0 error(s)
literals:
 - 2"1000_0001" -> (bit_string_literal)
 - 16"AFFE_Cafe" -> (bit_string_literal)
 - 8"777" -> (bit_string_literal)
 - 16"" -> (bit_string_literal)
================================================================================
parse success: true, 0 error(s)
literals:
 - 10#42# -> (integer_type, decimal_literal)
 - 10#1#e+3 -> (integer_type, decimal_literal)
 - 10#42.42# -> (real_type, decimal_literal)
 - 10#2.2#e-6 -> (real_type, decimal_literal)
 - 10#3.14#e+1 -> (real_type, decimal_literal)
================================================================================
parse success: true, 0 error(s)
literals:
 - 4#1_20#e1 -> (integer_type, based_literal)
 - 8#1_20#e1 -> (integer_type, based_literal)
 - 2#1100_0001# -> (integer_type, based_literal)
 - 10#42#e4 -> (integer_type, based_literal)
 - 16#AFFE_1.0Cafe# -> (real_type, based_literal)
 - 16#AFFE_2.0Cafe#e-10 -> (real_type, based_literal)
 - 16#DEAD_BEEF#e+0 -> (integer_type, based_literal)
================================================================================
parse success: true, 0 error(s)
literals:
 - 'setup time too small' -> (string_literal)
 - ' ' -> (string_literal)
 - 'a' -> (string_literal)
 - '""' -> (string_literal)
 - '' -> (string_literal)
================================================================================
parse success: true, 0 error(s)
literals:
 - "0" -> (character_literal, enumeration_literal)
 - "A" -> (character_literal, enumeration_literal)
 - "*" -> (character_literal, enumeration_literal)
 - "'" -> (character_literal, enumeration_literal)
 - " " -> (character_literal, enumeration_literal)
================================================================================
parse success: true, 0 error(s)
literals:
 - 10#10.7# -> (real_type, decimal_literal) [ns] -> (physical_literal)
 - 10#42# -> (integer_type, decimal_literal) [us] -> (physical_literal)
 - 10#42#e4 -> (integer_type, based_literal) [kg] -> (physical_literal)
================================================================================
)";

} // namespace testsuite_data

BOOST_AUTO_TEST_SUITE(literal_parser_success)

BOOST_AUTO_TEST_CASE(basic_parser_success)
{
    using stream_type = boost::test_tools::output_test_stream;
    auto os = stream_type{};

    for(auto idx = 0; auto const& input : testsuite_data::success_input) {
        reset_error_counter();
        ast::literals literals;
        bool parse_ok = parse(input, literals, os);
        BOOST_TEST(parse_ok == true);
        if(!literals.empty()) {
            os << "literals:\n";
            for (auto const& lit : literals) {
                os << " - " << lit << '\n';
            }
        }
        os << std::string(80, '=') << "\n";
    }

    auto const os_str = os.str();
    BOOST_TEST(!os.is_empty());
    BOOST_TEST(os_str == testsuite_data::os_expect);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BOOST_AUTO_TEST_SUITE_END()
