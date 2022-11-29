#include "rv64.hpp"

#include <array>

namespace arch::rv64 {
    class regfile {
        std::array<uint64_t, magic_enum::enum_count<reg>()> file{};

        public:
        uint64_t read(reg idx) const;
        void write(reg idx, uint64_t val);

        std::ostream& print(std::ostream& os) const;
    };
}
