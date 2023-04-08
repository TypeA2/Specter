#include "ir.hpp"

namespace arch::rv64 {
    abstract_reg instruction_parser::assign_to(rv64::reg rd) {
        _reg_state[rd] = _cur_reg;

        return _cur_reg++;
    }

    abstract_reg instruction_parser::read_from(rv64::reg rs) {
        return _reg_state.at(rs);
    }

    instruction_parser::parse_result instruction_parser::parse(const decoder& dec) {
        switch (dec.opcode()) {
            case opc::addi: {
                if (dec.rs1() == reg::zero) {
                    /* li or nop */
                    if (dec.rd() == reg::zero && dec.imm() == 0) {
                        return make_result<ir::nop>();
                    } else {
                        return make_result<ir::li>(assign_to(dec.rd()), dec.simm());
                    }
                } else {
                    return make_result<ir::addi>(assign_to(dec.rd()), read_from(dec.rs1()), dec.simm());
                }
                break;
            }

            case opc::ecall: {
                if (dec.imm()) {

                } else {
                    return make_result<ir::ecall>();
                }
                break;
            }

            default:
                return nullptr;
        }

        throw std::runtime_error("unknown opc");
    }
}
