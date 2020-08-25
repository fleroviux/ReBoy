/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "memory.hpp"

#include <cstdio>
#include <fstream>
#include <cstring>

void Memory::Reset() {
  std::memset(rom, 0, 0x8000);
  std::memset(wram, 0, 0x2000);
  std::memset(hram, 0, 0x7F);
  bootrom_disable = false;
}

auto Memory::ReadByte(std::uint16_t address) -> std::uint8_t {
  switch (address >> 12) {
    // ROM and External RAM
    case 0x0 ... 0x7:
    case 0xA ... 0xB: {
      if (!bootrom_disable && address <= 0xFF)
        return boot[address];
      if (mapper != nullptr)
        return mapper->Read(address);
      return 0xFF;
    }

    // VRAM
    case 0x8 ... 0x9: {
      return ppu->ReadVRAM(address & 0x1FFF);
    }

    // Work RAM
    case 0xC ... 0xD: {
      return wram[address & 0x1FFF];
    }

    // ECHO
    case 0xE: {
      return wram[address & 0xFFF];
    }

    case 0xF: {
      // ECHO
      if (address <= 0xFDFF)
        return wram[0x1000 + (address & 0xDFF)];
      // OAM
      if (address <= 0xFE9F) {
        return ppu->ReadOAM(address & 0x9F);
      }
      // Unusable
      if (address <= 0xFEFF) {
        std::puts("Unhandled read from unused memory!");
        return 0;
      }
      // MMIO
      if (address <= 0xFF7F || address == 0xFFFF) {
        return ReadMMIO(address & 0xFF);
      }
      // HRAM
      return hram[address & 0x7F];
    }
  }
}

void Memory::WriteByte(std::uint16_t address, std::uint8_t value) {
  switch (address >> 12) {
    // ROM and External RAM
    case 0x0 ... 0x7:
    case 0xA ... 0xB: {
      if (mapper != nullptr)
        mapper->Write(address, value);
      break;
    }

    // VRAM
    case 0x8 ... 0x9: {
      ppu->WriteVRAM(address & 0x1FFF, value);
      break;
    }

    // Work RAM
    case 0xC:
    case 0xD: {
      wram[address & 0x1FFF] = value;
      break;
    }

      // ECHO
    case 0xE: {
      wram[address & 0xFFF] = value;
      break;
    }

    case 0xF: {
      // ECHO
      if (address <= 0xFDFF) {
        wram[0x1000 + (address & 0xDFF)] = value;
      }
      // OAM
      else if (address <= 0xFE9F) {
        ppu->WriteOAM(address & 0x9F, value);
      }
      // Unusable
      else if (address <= 0xFEFF) {
        std::puts("Unhandled write to unused memory!");
      }
      // MMIO
      else if (address <= 0xFF7F || address == 0xFFFF) {
        WriteMMIO(address & 0xFF, value);
      }
      // HRAM
      else
        hram[address & 0x7F] = value;
      break;
    }
  }
}

auto Memory::ReadMMIO(std::uint8_t reg) -> std::uint8_t {
  if (reg == 0)
    return joypad->Read();

  if (reg >= kTimerMinReg && reg <= kTimerMaxReg)
    return timer->ReadMMIO(reg);

  if (reg >= kAPUMinReg && reg <= kAPUMaxReg)
    return apu->ReadMMIO(reg);

  if (reg >= kPPUMinReg && reg <= kPPUMaxReg)
    return ppu->ReadMMIO(reg);

  if (reg == IRQ::REG_IE || reg == IRQ::REG_IF)
    return irq->ReadMMIO(reg);

  std::printf("Unhandled MMIO read from 0xFF%02X\n", reg);
  return 0x0;
}

void Memory::WriteMMIO(std::uint8_t reg, std::uint8_t value) {
  if (reg == 0) {
    joypad->Write(value);
    return;
  }

  if (reg >= kTimerMinReg && reg <= kTimerMaxReg) {
    timer->WriteMMIO(reg, value);
    return;
  }

  if (reg >= kAPUMinReg && reg <= kAPUMaxReg) {
    apu->WriteMMIO(reg, value);
    return;
  }

  if (reg >= kPPUMinReg && reg <= kPPUMaxReg) {
    // OAM DMA transfer
    // TODO: this is very inaccurate right now.
    // Timing emulation is completely missing, as well as blocking out unavailable memory.
    // Should we move this code to the PPU?
    if (reg == 0x46) {
      std::uint16_t src = value << 8;
      for (std::uint8_t i = 0; i < 0xA0; i++)
        ppu->WriteOAM(i, ReadByte(src++));
      return;
    }
    ppu->WriteMMIO(reg, value);
    return;
  }

  if (reg == IRQ::REG_IE || reg == IRQ::REG_IF) {
    irq->WriteMMIO(reg, value);
    return;
  }

  if (reg == 0x50) {
    bootrom_disable = value & 1;
    return;
  }

  std::printf("Unhandled MMIO write to 0xFF%02X = 0x%02X\n", reg, value);
}
