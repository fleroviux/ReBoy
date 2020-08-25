/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdio>

#include "memory.hpp"

class CPU {
public:
  CPU(MemoryBase* memory);

  void Reset();
  void Step();
  void RaiseIRQ(std::uint8_t vector);

  bool interrupt_master_enable;

private:
  MemoryBase* memory;

  enum class RegB {
    A, F, B, C, D, E, H, L
  };

  enum class RegW {
    AF, BC, DE, HL, SP, PC
  };

  enum class Flag : std::uint8_t {
    Carry = 0x10,
    HalfCarry = 0x20,
    Negative = 0x40,
    Zero = 0x80
  };

  auto GetRegB(RegB reg) -> std::uint8_t&;
  auto GetRegW(RegW reg) -> std::uint16_t&;
  void SetFlag(Flag flag, bool is_set);
  auto GetFlag(Flag flag) -> bool;

  // TODO: add support for big-endian systems.
  union GPR {
    std::uint16_t word;
    struct {
      std::uint8_t lo;
      std::uint8_t hi;
    } byte;
  } af, bc, de, hl;
  std::uint16_t sp;
  std::uint16_t pc;

  bool halted = false;

  void Push(std::uint16_t value) {
    sp -= 2;
    memory->WriteWord(sp, value);
  }

  auto Pop() -> std::uint16_t {
    auto value = memory->ReadWord(sp);
    sp += 2;
    return value;
  }

  #include "instructions.inc"

  static void (CPU::*sOpcodeTable[256])(void);
  static void (CPU::*sOpcodeTableCB[256])(void);
};