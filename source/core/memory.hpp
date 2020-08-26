/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include "apu/apu.hpp"
#include "cpu/memory.hpp"
#include "irq.hpp"
#include "ppu/ppu.hpp"
#include "mbc/mbc.hpp"
#include "joypad.hpp"
#include "timer.hpp"

class Memory : public MemoryBase {
public:
  Memory(Scheduler* scheduler, IRQ* irq, PPU* ppu, APU* apu, Timer* timer, Joypad* joypad)
    : scheduler(scheduler), irq(irq), ppu(ppu), apu(apu), timer(timer), joypad(joypad) { Reset(); }

  void Reset();
  auto ReadByte(std::uint16_t address) -> std::uint8_t;
  void WriteByte(std::uint16_t address, std::uint8_t value);
  auto GetROM1Bank() -> std::uint8_t override { return mapper == nullptr ? 1 : mapper->GetROM1Bank(); }

  /// BOOTROM memory region
  std::uint8_t boot[0x100];
  std::uint8_t boot_cgb[0x700];
  bool enable_is_cgb = false;

  /// GamePak MBC + ROM + SRAM
  MBCBase* mapper = nullptr;

private:
  std::uint8_t rom[0x8000];
  std::uint8_t wram[8][0x1000];
  std::uint8_t hram[0x7F];
  Scheduler* scheduler;
  IRQ* irq;
  PPU* ppu;
  APU* apu;
  Timer* timer;
  Joypad* joypad;

  bool bootrom_disable;
  int wram_bank;
  int vram_bank;

  auto ReadMMIO(std::uint8_t reg) -> std::uint8_t;
  void WriteMMIO(std::uint8_t reg, std::uint8_t value);

  static constexpr std::uint8_t kTimerMinReg = 0x04;
  static constexpr std::uint8_t kTimerMaxReg = 0x07;
  static constexpr std::uint8_t kAPUMinReg = 0x10;
  static constexpr std::uint8_t kAPUMaxReg = 0x3F;
  static constexpr std::uint8_t kPPUMinReg = 0x40;
  static constexpr std::uint8_t kPPUMaxReg = 0x4B;
};
