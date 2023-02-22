#include "alu.hpp"

namespace arch::rv64 {
    void alu::pulse() {
        switch (_op) {
            case alu_op::invalid:
                throw invalid_alu_op(_a, _b, _op);

            case alu_op::nop:
                break;

            case alu_op::forward_a:
                _res = _a;
                break;

            case alu_op::forward_b:
                _res = _b;
                break;
            
            case alu_op::add:
                _res = _a + _b;
                break;

            case alu_op::sub:
                _res = int64_t(_a) - int64_t(_b);
                break;

            case alu_op::div:
                _res = int64_t(_a) / int64_t(_b);
                break;

            case alu_op::divu:
                _res = _a / _b;
                break;

            case alu_op::mul:
                _res = int64_t(_a) * int64_t(_b);
                break;

            case alu_op::rem:
                _res = int64_t(_a) % int64_t(_b);
                break;

            case alu_op::remu:
                _res = _a % _b;
                break;

            case alu_op::addw:
                _res = sign_extend<32>((_a + _b) & 0xffffffff);
                break;

            case alu_op::subw:
                _res = sign_extend<32>(uint64_t(int64_t(_a) - int64_t(_b)) & 0xffffffff);
                break;

            case alu_op::eq:
                _res = (_a == _b) ? 1 : 0;
                break;

            case alu_op::ne:
                _res = (_a != _b) ? 1 : 0;
                break;

            case alu_op::lt:
                _res = (int64_t(_a) < int64_t(_b)) ? 1 : 0;
                break;

            case alu_op::ge:
                _res = (int64_t(_a) >= int64_t(_b)) ? 1 : 0;
                break;

            case alu_op::ltu:
                _res = (_a < _b) ? 1 : 0;
                break;

            case alu_op::geu:
                _res = (_a >= _b) ? 1 : 0;
                break;

            case alu_op::bitwise_xor:
                _res = _a ^ _b;
                break;

            case alu_op::bitwise_or:
                _res = _a | _b;
                break;

            case alu_op::bitwise_and:
                _res = _a & _b;
                break;

            case alu_op::sll:
                _res = _a << (_b & 0b111111);
                break;

            case alu_op::srl:
                _res = _a >> (_b & 0b111111);
                break;

            case alu_op::sra:
                _res = int64_t(_a) >> (_b & 0b111111);
                break;

            case alu_op::sllw:
                _res = sign_extend<32>((_a << (_b & 0b11111)) & 0xffffffff);
                break;

            case alu_op::srlw:
                _res = sign_extend<32>((_a >> (_b & 0b111111)) & 0xffffffff);
                break;

            case alu_op::sraw:
                _res = sign_extend<32>(static_cast<uint64_t>(int64_t(_a) >> (_b & 0b111111)) & 0xffffffff);
                break;
        }
    }
}
