/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include "cpu/cpu.hpp"

class IRQ {
public:
  enum Registers {
    REG_IF = 0x0F,
    REG_IE = 0xFF
  };

  enum Interrupts {
    VBLANK = 1,
    LCD_STAT = 2,
    TIMER = 4,
    SERIAL = 8,
    JOYPAD = 16
  };

  IRQ(CPU* cpu);

  void Reset();
  void Step();
  void Raise(Interrupts irq);
  auto ReadMMIO(std::uint8_t reg) -> std::uint8_t;
  void WriteMMIO(std::uint8_t reg, std::uint8_t value);

private:
  CPU* cpu;
  std::uint8_t _ie;
  std::uint8_t _if;
};
