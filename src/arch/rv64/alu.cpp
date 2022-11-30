#include "alu.hpp"

namespace arch::rv64 {
    void alu::pulse() {
        switch (_op) {
            case alu_op::invalid: {
                throw invalid_alu_op(_a, _b, _op);
            }
            
            case alu_op::add: {
                _res = _a + _b;
                break;
            }

            case alu_op::lt: {
                _res = (int64_t(_a) < int64_t(_b)) ? 1 : 0;
                break;
            }

            case alu_op::ltu: {
                _res = (_a < _b) ? 1 : 0;
                break;
            }
        }
    }
}
