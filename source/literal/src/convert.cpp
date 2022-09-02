#include <literal/convert/convert.hpp>

#include <climits> // CHAR_BIT
#include <cstdint>
#include <array>

namespace convert {

namespace detail {

std::uint32_t chr2dec(char chr)
{
    static auto constexpr alnum_to_value_table = []() {
        unsigned char constexpr lower_letters[] = "abcdefghijklmnopqrstuvwxyz";
        unsigned char constexpr upper_letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        unsigned constexpr N = 1U << CHAR_BIT;
        std::array<unsigned char, N> table{}; // works with any char type

        for (std::size_t i : table) {
            table[i] = 0x7F;
        }
        for (std::size_t i = 0; i != 10; ++i) {
            table['0' + i] = static_cast<unsigned char>(i);
        }
        for (std::size_t i = 0; i != 26; ++i) {
            table[lower_letters[i]] = 10 + static_cast<unsigned char>(i);
            table[upper_letters[i]] = 10 + static_cast<unsigned char>(i);
        }
        return table;
    }();

    return alnum_to_value_table[static_cast<unsigned char>(chr)];
}

}  // namespace detail
}  // namespace convert
