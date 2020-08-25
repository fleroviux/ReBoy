/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "irq.hpp"

IRQ::IRQ(CPU* cpu) : cpu(cpu) {
  Reset();
}

void IRQ::Reset() {
  _ie = 0;
  _if = 0;
}

void IRQ::Step() {
  constexpr std::uint8_t kIRQVectors[] = {
          0x40, 0x48, 0x50, 0x58, 0x60 };
  auto enabled_and_requested = _ie & _if;
  if (!cpu->interrupt_master_enable || enabled_and_requested == 0)
    return;
  for (int i = 0; i <= 4; i++) {
    if (enabled_and_requested & (1 << i)) {
      cpu->RaiseIRQ(kIRQVectors[i]);
      _if &= ~(1 << i);
      break;
    }
  }
}

void IRQ::Raise(Interrupts irq) {
  _if |= irq;
}

auto IRQ::ReadMMIO(std::uint8_t reg) -> std::uint8_t {
  if (reg == REG_IF)
    return _if;
  return _ie;
}

void IRQ::WriteMMIO(std::uint8_t reg, std::uint8_t value) {
  if (reg == REG_IF)
    _if = value;
  else
    _ie = value;
}
