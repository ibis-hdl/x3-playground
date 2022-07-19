#include <literal/convert.hpp>

#include <climits> // CHAR_BIT
#include <cstdint>
#include <array>

namespace convert {

namespace detail {

std::uint32_t chr2dec(char chr)
{
    // Construct a lookup table which maps '0-9', 'A-Z' and 'a-z' to their
    // corresponding base (until 36) value and maps all other characters to 0x7F
    // (7-Bit ASCII 127d 'delete') nevertheless of concrete type of char
    // (byte-size, sign).
    // concept [coliru](https://godbolt.org/z/EvEnKqxox)
    static auto constexpr alnum_to_value_table = []() {
        unsigned char constexpr lower_letters[] = "abcdefghijklmnopqrstuvwxyz";
        unsigned char constexpr upper_letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        unsigned constexpr N = 1U << CHAR_BIT;
        std::array<unsigned char, N> table{};

        for (std::size_t i : table) {
            table[i] = 0x7F;
        }
        for (std::size_t i = 0; i != 10; ++i) {
            table['0' + i] = i;
        }
        for (std::size_t i = 0; i != 26; ++i) {
            table[lower_letters[i]] = 10 + i;
            table[upper_letters[i]] = 10 + i;
        }
        return table;
    }();

    return alnum_to_value_table[chr];
}

}  // namespace detail
}  // namespace convert
