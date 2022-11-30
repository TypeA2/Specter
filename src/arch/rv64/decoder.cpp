#include "decoder.hpp"

template <typename T, size_t N, typename E> requires std::is_enum_v<E>
class enum_array {
    std::array<T, N> val{};

    public:
    constexpr enum_array() = default;

    [[nodiscard]] constexpr std::array<T, N> get() {
        return std::move(val);
    }

    [[nodiscard]] constexpr T& operator[](size_t idx) {
        return val[idx];
    }

    [[nodiscard]] constexpr T& operator[](E idx) {
        return val[magic_enum::enum_integer(idx)];
    }

    struct proxy2d {
        T& target;

        constexpr proxy2d& operator()(uint64_t idx, typename T::value_type val) {
            target[idx] = val;
            return *this;
        }
    };

    [[nodiscard]] constexpr proxy2d set_for(E idx) {
        return { (*this)[idx] };
    }
};

namespace arch::rv64 {
    void decoder::_decode_full() {
        _opcode = static_cast<opc>(_instr & 0x7f);

        switch (_opcode) {
            case opc::load:
            case opc::addi:
                _type = instr_type::I;
                _decode_i();
                break;

            case opc::ecall: {
                _type = instr_type::I;
                /* ecall only has 1 bit in it's immediate */
                _imm = (_instr >> 20) & 1;
                break;
            }

            default:
                throw illegal_instruction(_pc, _instr, "decode");
        }

        /* These depend on values from the previously executed decode (for the most part) */
        // TODO maybe rework to only switch on _opcode once? 
        switch (_opcode) {
            case opc::load:
                _is_memory = true;
                _unsigned_memory = (_funct & 0b100) ? true : false;
                _memory_size = 1 << (_funct & 0b11);
                break;

            default:
                _is_memory = false;
        }
    }

    static constexpr auto alu_i_op_map() {
        enum_array<std::array<alu_op, 8>, 128, opc> res;

        res.set_for(opc::load)
            (0b000, alu_op::add) /* lb */
            (0b001, alu_op::add) /* lh */
            (0b010, alu_op::add) /* lw */
            (0b011, alu_op::add) /* ld */
            (0b100, alu_op::add) /* lbu */
            (0b101, alu_op::add) /* lhu */
            (0b110, alu_op::add) /* lwu */
            ;

        res.set_for(opc::addi)
            (0b000, alu_op::add) /* addi */
            (0b010, alu_op::lt)  /* slti */
            (0b011, alu_op::ltu) /* sltiu */
            ;

        return res;
    }

    void decoder::_decode_i() {
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _imm = sign_extend<12>((_instr >> 20) & 0xfff);

        static constinit auto alu_op_map = alu_i_op_map();

        _op = alu_op_map[_opcode][_funct];
    }

    void decoder::_decode_compressed() {
        throw illegal_compressed_instruction(_pc, _instr, "decode compressed");
    }

    void decoder::set_instr(uintptr_t pc, uint32_t instr) {
        _instr = instr;
        _pc = pc;
        _compressed = !((_instr & 0b11) == OPC_FULL_SIZE);
        
        if (_compressed) {
            _decode_compressed();
        } else {
            _decode_full();
        }
    }
}
