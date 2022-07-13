#include <literal/convert.hpp>

#include <cstdint>
#include <array>

namespace convert {

namespace detail {

std::uint32_t chr2dec(char chr)
{
    // Construct a lookup table which maps '0-9', 'A-Z' and 'a-z' to their
    // corresponding base (until 36) value and maps all other characters to 0x7F
    // (signed char 127; ascii 'delete') nevertheless of use byte-size or sign of char.
    // concept [coliru](https://godbolt.org/z/EvEnKqxox)
    static auto constexpr alnum_to_value_table = []()
    {
        unsigned char constexpr lower_letters[] = "abcdefghijklmnopqrstuvwxyz";
        unsigned char constexpr upper_letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        unsigned constexpr N = 1U << CHAR_BIT;
        std::array<unsigned char, N> table {};

        for (auto i : table) {
            table[i] = 0x7F;
        }
        for (int i = 0; i != 10; ++i) {
            table['0' + i] = i;
        }
        for (int i = 0; i != 26; ++i) {
            table[lower_letters[i]] = 10 + i;
            table[upper_letters[i]] = 10 + i;
        }
        return table;
    }();

    return alnum_to_value_table[chr];
}

}}
