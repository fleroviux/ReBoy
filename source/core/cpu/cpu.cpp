/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "cpu.hpp"

CPU::CPU(MemoryBase* memory) : memory(memory) {
  Reset();
}

void CPU::Reset() {
  GetRegW(RegW::AF) = 0;
  GetRegW(RegW::BC) = 0;
  GetRegW(RegW::DE) = 0;
  GetRegW(RegW::HL) = 0;
  GetRegW(RegW::SP) = 0;
  GetRegW(RegW::PC) = 0;
  interrupt_master_enable = false;
}

void CPU::RaiseIRQ(std::uint8_t vector) {
  halted = false;
  interrupt_master_enable = false;
  Push(pc);
  pc = vector;
}

auto CPU::GetRegB(RegB reg) -> std::uint8_t& {
  switch (reg) {
    case RegB::A:
      return af.byte.hi;
    case RegB::F:
      return af.byte.lo;
    case RegB::B:
      return bc.byte.hi;
    case RegB::C:
      return bc.byte.lo;
    case RegB::D:
      return de.byte.hi;
    case RegB::E:
      return de.byte.lo;
    case RegB::H:
      return hl.byte.hi;
    case RegB::L:
      return hl.byte.lo;
  }
}

auto CPU::GetRegW(RegW reg) -> std::uint16_t& {
  switch (reg) {
    case RegW::AF:
      return af.word;
    case RegW::BC:
      return bc.word;
    case RegW::DE:
      return de.word;
    case RegW::HL:
      return hl.word;
    case RegW::SP:
      return sp;
    case RegW::PC:
      return pc;
  }
}

void CPU::SetFlag(Flag flag, bool is_set) {
  auto& f = GetRegB(RegB::F);
  if (is_set) {
    f |= static_cast<std::uint8_t>(flag);
  } else {
    f &= ~static_cast<std::uint8_t>(flag);
  }
}

auto CPU::GetFlag(Flag flag) -> bool {
  return GetRegB(RegB::F) & static_cast<std::uint8_t>(flag);
}

void CPU::Step() {
  if (halted)
    return;
  auto opcode = memory->ReadByte(GetRegW(RegW::PC)++);
  (this->*sOpcodeTable[opcode])();
}
