/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

enum class OpMode {
  Reg,
  Reg16,
  Imm,
  Imm16,
  PointerReg16,
  PointerReg16Inc,
  PointerReg16Dec,
  PointerWord,
  Pointer16Word,
  HighMemImm,
  HighMemC
};

enum class Reg {
  None, A, B, C, D, E, H, L, BC, DE, HL, SP
};

template <OpMode mode, Reg reg = Reg::None>
struct Operand {
  Operand(CPU* cpu) : cpu(cpu) {
    switch (mode) {
      case OpMode::Imm:
      case OpMode::HighMemImm:
        imm = cpu->memory->ReadByte(cpu->pc++);
        break;
      case OpMode::Imm16:
      case OpMode::PointerWord:
      case OpMode::Pointer16Word:
        imm16 = cpu->memory->ReadWord(cpu->pc);
        cpu->pc += 2;
        break;
    }
  }

  explicit operator std::uint8_t() {
    switch (mode) {
      case OpMode::Reg:
        return cpu->GetRegB(GetRegByteFromOperandReg());
      case OpMode::Imm:
        return imm;
      case OpMode::PointerReg16:
        return cpu->memory->ReadByte(cpu->GetRegW(GetRegWordFromOperandReg()));
      case OpMode::PointerReg16Inc:
        return cpu->memory->ReadByte(cpu->GetRegW(GetRegWordFromOperandReg())++);
      case OpMode::PointerReg16Dec:
        return cpu->memory->ReadByte(cpu->GetRegW(GetRegWordFromOperandReg())--);
      case OpMode::PointerWord:
        return cpu->memory->ReadByte(imm16);
      case OpMode::HighMemImm:
        return cpu->memory->ReadByte(0xFF00 + imm);
      case OpMode::HighMemC:
        return cpu->memory->ReadByte(0xFF00 + cpu->GetRegB(RegB::C));
    }
    return 0;
  }

  explicit operator std::uint16_t() {
    switch (mode) {
      case OpMode::Reg16:
        return cpu->GetRegW(GetRegWordFromOperandReg());
      case OpMode::Imm16:
        return imm16;
      case OpMode::Pointer16Word:
        return cpu->memory->ReadWord(imm16);
    }
    return 0;
  }

  void operator=(std::uint8_t value) {
    switch (mode) {
      case OpMode::Reg:
        cpu->GetRegB(GetRegByteFromOperandReg()) = value;
        break;
      case OpMode::PointerReg16:
        cpu->memory->WriteByte(cpu->GetRegW(GetRegWordFromOperandReg()), value);
        break;
      case OpMode::PointerReg16Inc:
        cpu->memory->WriteByte(cpu->GetRegW(GetRegWordFromOperandReg())++, value);
        break;
      case OpMode::PointerReg16Dec:
        cpu->memory->WriteByte(cpu->GetRegW(GetRegWordFromOperandReg())--, value);
        break;
      case OpMode::PointerWord:
        cpu->memory->WriteByte(imm16, value);
        break;
      case OpMode::HighMemImm:
        cpu->memory->WriteByte(0xFF00 + imm, value);
        break;
      case OpMode::HighMemC:
        cpu->memory->WriteByte(0xFF00 + cpu->GetRegB(RegB::C), value);
        break;
    }
  }

  void operator=(std::uint16_t value) {
    if (mode == OpMode::Reg16)
      cpu->GetRegW(GetRegWordFromOperandReg()) = value;
    else
      cpu->memory->WriteWord(imm16, value);
  }

private:
  constexpr auto GetRegByteFromOperandReg() -> RegB {
    // TODO: throw compile-time error if reg value isn't valid.
    switch (reg) {
      case Reg::A: return RegB::A;
      case Reg::B: return RegB::B;
      case Reg::C: return RegB::C;
      case Reg::D: return RegB::D;
      case Reg::E: return RegB::E;
      case Reg::H: return RegB::H;
      case Reg::L: return RegB::L;
    }
  }

  constexpr auto GetRegWordFromOperandReg() -> RegW {
    // TODO: throw compile-time error if reg value isn't valid.
    switch (reg) {
      case Reg::BC: return RegW::BC;
      case Reg::DE: return RegW::DE;
      case Reg::HL: return RegW::HL;
      case Reg::SP: return RegW::SP;
    }
  }

  CPU* cpu;
  std::uint8_t  imm;
  std::uint16_t imm16;
};

enum class GenericOp {
  Move,
  Move16,
  Increment,
  Increment16,
  Decrement,
  Decrement16,
  Add,
  AddWithCarry,
  Add16,
  Sub,
  SubWithCarry,
  AND,
  XOR,
  OR,
  Compare,
  RLC,
  RRC,
  RL,
  RR,
  SLA,
  SRA,
  SWAP,
  SRL,
  BIT,
  RES,
  SET
};

template <GenericOp operation,
        OpMode op1_mode,
        Reg  op1_reg,
        OpMode op2_mode,
        Reg  op2_reg = Reg::None,
        unsigned cb_bit = 0>
void GenerateOpcode() {
  Operand<op1_mode, op1_reg> op1 {this};
  Operand<op2_mode, op2_reg> op2 {this};

  const auto Add = [&](std::uint8_t op1, std::uint8_t op2, int carry) -> std::uint8_t {
    std::uint8_t result = op1 + op2 + carry;
    SetFlag(Flag::Negative, false);
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::HalfCarry, ((op1 & 0xF) + (op2 & 0xF) + carry) & 0x10);
    SetFlag(Flag::Carry,(std::uint16_t(op1) + (std::uint16_t(op2) + carry)) & 0x100);
    return result;
  };

  const auto Add16 = [&](std::uint16_t op1, std::uint16_t op2) -> std::uint16_t {
    std::uint16_t result = op1 + op2;
    SetFlag(Flag::Negative, false);
    SetFlag(Flag::HalfCarry, ((op1 & 0xFFF) + (op2 & 0xFFF)) & 0x1000);
    SetFlag(Flag::Carry, result < op1);
    return result;
  };

  const auto Subtract = [&](std::uint8_t op1, std::uint8_t op2, int carry) -> std::uint8_t {
    std::uint8_t result = op1 - op2 - carry;
    SetFlag(Flag::Negative, true);
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::HalfCarry, (op1 & 0xF) < ((op2 & 0xF) + carry));
    SetFlag(Flag::Carry, op1 < (op2 + carry));
    return result;
  };

  switch (operation) {
    case GenericOp::Move:
      op1 = std::uint8_t(op2);
      break;
    case GenericOp::Move16:
      op1 = std::uint16_t(op2);
      break;
    case GenericOp::Increment: {
      std::uint8_t result = std::uint8_t(op2) + 1;
      SetFlag(Flag::Zero, result == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, (std::uint8_t(op2) & 0xF) == 0xF);
      op1 = result;
      break;
    }
    case GenericOp::Increment16:
      op1 = std::uint16_t(std::uint16_t(op2) + 1);
      break;
    case GenericOp::Decrement: {
      std::uint8_t result = std::uint8_t(op2) - 1;
      SetFlag(Flag::Zero, result == 0);
      SetFlag(Flag::Negative, true);
      SetFlag(Flag::HalfCarry, (std::uint8_t(op2) & 0xF) == 0);
      op1 = result;
      break;
    }
    case GenericOp::Decrement16:
      op1 = std::uint16_t(std::uint16_t(op2) - 1);
      break;
    case GenericOp::Add:
      op1 = Add(std::uint8_t(op1), std::uint8_t(op2), 0);
      break;
    case GenericOp::AddWithCarry:
      op1 = Add(std::uint8_t(op1), std::uint8_t(op2), GetFlag(Flag::Carry) ? 1 : 0);
      break;
    case GenericOp::Add16:
      op1 = Add16(std::uint16_t(op1), std::uint16_t(op2));
      break;
    case GenericOp::Sub:
      op1 = Subtract(std::uint8_t(op1), std::uint8_t(op2), 0);
      break;
    case GenericOp::SubWithCarry:
      op1 = Subtract(std::uint8_t(op1), std::uint8_t(op2), GetFlag(Flag::Carry) ? 1 : 0);
      break;
    case GenericOp::AND:
      op1 = std::uint8_t(std::uint8_t(op1) & std::uint8_t(op2));
      SetFlag(Flag::Zero, std::uint8_t(op1) == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, true);
      SetFlag(Flag::Carry, false);
      break;
    case GenericOp::XOR:
      op1 = std::uint8_t(std::uint8_t(op1) ^ std::uint8_t(op2));
      SetFlag(Flag::Zero, std::uint8_t(op1) == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, false);
      break;
    case GenericOp::OR:
      op1 = std::uint8_t(std::uint8_t(op1) | std::uint8_t(op2));
      SetFlag(Flag::Zero, std::uint8_t(op1) == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, false);
      break;
    case GenericOp::Compare:
      Subtract(std::uint8_t(op1), std::uint8_t(op2), 0);
      break;
    case GenericOp::RLC: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value >> 7;
      value = (value << 1) | carry;
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::RRC: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value & 1;
      value = (value >> 1) | (carry << 7);
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::RL: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value >> 7;
      value = (value << 1) | (GetFlag(Flag::Carry) ? 1 : 0);
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::RR: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value & 1;
      value = (value >> 1) | (GetFlag(Flag::Carry) ? 0x80 : 0);
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::SLA: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value >> 7;
      value <<= 1;
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::SRA: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value & 1;
      value = (value & 0x80) | (value >> 1);
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::SWAP: {
      std::uint8_t value = std::uint8_t(op2);
      value = (value << 4) | (value >> 4);
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, false);
      op1 = value;
      break;
    }
    case GenericOp::SRL: {
      std::uint8_t value = std::uint8_t(op2);
      std::uint8_t carry = value & 1;
      value >>= 1;
      SetFlag(Flag::Zero, value == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, false);
      SetFlag(Flag::Carry, carry);
      op1 = value;
      break;
    }
    case GenericOp::BIT:
      SetFlag(Flag::Zero, (std::uint8_t(op2) & (1 << cb_bit)) == 0);
      SetFlag(Flag::Negative, false);
      SetFlag(Flag::HalfCarry, true);
      break;
    case GenericOp::RES:
      op1 = std::uint8_t(std::uint8_t(op2) & ~(1 << cb_bit));
      break;
    case GenericOp::SET:
      op1 = std::uint8_t(std::uint8_t(op2) | (1 << cb_bit));
      break;
  }
}

void NOP() {}

void UNKNOWN() {
  auto opcode = memory->ReadByte(GetRegW(RegW::PC) - 1);
  std::printf("bad opcode 0x%02X\n", opcode);
  for (;;);
}

template <RegW reg>
void PUSH_R16() {
  Push(GetRegW(reg));
}

template <RegW reg>
void POP_R16() {
  auto value= Pop();
  if (reg == RegW::AF)
    value &= 0xFFF0;
  GetRegW(reg) = value;
}

/// Control flow
void JR_S8() {
  pc += std::int8_t(memory->ReadByte(pc++));
}

template <Flag flag, bool set>
void JR_COND_S8() {
  if (GetFlag(flag) == set)
    JR_S8();
  else
    pc++;
}

void JP_U16() {
  pc = memory->ReadWord(pc);
}

template <Flag flag, bool set>
void JP_COND_U16() {
  if (GetFlag(flag) == set)
    JP_U16();
  else
    pc += 2;
}

void JP_HL() {
  pc = GetRegW(RegW::HL);
}

void CALL_U16() {
  Push(pc + 2);
  pc = memory->ReadWord(pc);
}

template <Flag flag, bool set>
void CALL_COND_U16() {
  if (GetFlag(flag) == set)
    CALL_U16();
  else
    pc += 2;
}

void RET() {
  pc = Pop();
}

void RETI() {
  interrupt_master_enable = true;
  pc = Pop();
}

void HALT() {
  // TODO: implement the infamous HALT-bug
  halted = true;
}

template <Flag flag, bool set>
void RET_COND() {
  if (GetFlag(flag) == set)
    RET();
}

template <std::uint8_t vector>
void RST() {
  Push(pc);
  pc = vector;
}

void PREFIX_CB() {
  (this->*sOpcodeTableCB[memory->ReadByte(pc++)])();
}

/// Signed SP offset
void ADD_SP_S8() {
  std::int16_t op2 = std::int16_t(std::int8_t(memory->ReadByte(pc++)));
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, ((GetRegW(RegW::SP) & 0xF) + (op2 & 0xF)) & 0x10);
  SetFlag(Flag::Carry, ((GetRegW(RegW::SP) & 0xFF) + (op2 & 0xFF)) & 0x100);
  GetRegW(RegW::SP) += op2;
}

void LD_HL_SP_S8() {
  std::int16_t op2 = std::int16_t(std::int8_t(memory->ReadByte(pc++)));
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, ((GetRegW(RegW::SP) & 0xF) + (op2 & 0xF)) & 0x10);
  SetFlag(Flag::Carry, ((GetRegW(RegW::SP) & 0xFF) + (op2 & 0xFF)) & 0x100);
  GetRegW(RegW::HL) = GetRegW(RegW::SP) + op2;
}

/// Interrupt control
void DI() {
  interrupt_master_enable = false;
}

void EI() {
  interrupt_master_enable = true;
}

/// Bitwise instructions
void RLCA() {
  auto& value = GetRegB(RegB::A);
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  SetFlag(Flag::Carry, value & 0x80);
  value = (value << 1) | (value >> 7);
}

void RRCA() {
  auto& value = GetRegB(RegB::A);
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  SetFlag(Flag::Carry, value & 1);
  value = (value >> 1) | (value << 7);
}

void RLA() {
  auto carry_in = (GetRegB(RegB::F) & static_cast<std::uint8_t>(Flag::Carry)) ? 1 : 0;
  auto& value = GetRegB(RegB::A);
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  SetFlag(Flag::Carry, value & 0x80);
  value = (value << 1) | carry_in;
}

void RRA() {
  auto carry_in = (GetRegB(RegB::F) & static_cast<std::uint8_t>(Flag::Carry)) ? 1 : 0;
  auto& value = GetRegB(RegB::A);
  SetFlag(Flag::Zero, false);
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  SetFlag(Flag::Carry, value & 0x1);
  value = (value >> 1) | (carry_in << 7);
}

void CPL() {
  GetRegB(RegB::A) ^= 0xFF;
  SetFlag(Flag::Negative, true);
  SetFlag(Flag::HalfCarry, true);
}

void CCF() {
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  GetRegB(RegB::F) ^= static_cast<std::uint8_t>(Flag::Carry);
}

void SCF() {
  SetFlag(Flag::Negative, false);
  SetFlag(Flag::HalfCarry, false);
  SetFlag(Flag::Carry, true);
}

void DAA() {
  // https://forums.nesdev.com/viewtopic.php?t=15944
  auto& a = GetRegB(RegB::A);
  auto n_flag = GetRegB(RegB::F) & static_cast<std::uint8_t>(Flag::Negative);
  auto c_flag = GetRegB(RegB::F) & static_cast<std::uint8_t>(Flag::Carry);
  auto h_flag = GetRegB(RegB::F) & static_cast<std::uint8_t>(Flag::HalfCarry);
  if (!n_flag) {
    if (c_flag || a > 0x90) {
      a += 0x60;
      SetFlag(Flag::Carry, true);
    }
    if (h_flag || (a & 0xF) > 0x09) {
      a += 0x6;
    }
  } else {
    if (c_flag) {
      a -= 0x60;
    }
    if (h_flag) {
      a -= 0x6;
    }
  }

  SetFlag(Flag::Zero, a == 0);
  SetFlag(Flag::HalfCarry, false);
}