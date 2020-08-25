/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

struct MemoryBase {
  virtual auto ReadByte(std::uint16_t address) -> std::uint8_t = 0;
  virtual void WriteByte(std::uint16_t address, std::uint8_t value) = 0;
  virtual auto GetROM1Bank() -> std::uint8_t = 0;

  auto ReadWord(std::uint16_t address) -> std::uint16_t {
    return ReadByte(address) | (ReadByte(address + 1) << 8);
  }

  void WriteWord(std::uint16_t address, std::uint16_t value) {
    WriteByte(address, value & 0xFF);
    WriteByte(address + 1, value >> 8);
  }
};