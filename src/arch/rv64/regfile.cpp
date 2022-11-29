#include "regfile.hpp"

namespace arch::rv64 {
    uint64_t regfile::read(reg idx) const {
        return file[static_cast<uint8_t>(idx)];
    }

    void regfile::write(reg idx, uint64_t val) {
        if (idx != reg::zero){
            file[static_cast<uint8_t>(idx)] = val;
        }
    }
}
