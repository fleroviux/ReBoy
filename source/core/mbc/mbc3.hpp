/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <memory>

#include "backup-file.hpp"
#include "mbc.hpp"

class MBC3 : public MBCBase {
public:
  MBC3(std::unique_ptr<std::uint8_t[]> data, size_t size, std::string const& path)
    : data(std::move(data)), size(size)
  {
    int unused = 0x8000;
    sram = BackupFile::OpenOrCreate(path, { 0x8000 }, unused);
  }

  auto GetROM1Bank() -> std::uint8_t override { return rom_bank; }

  auto Read(std::uint16_t address) -> std::uint8_t override {
    switch (address >> 12) {
      // ROM bank 0
      case 0x0 ... 0x3:
        // NOTE: this isn't strictly necessary because the Gameboy class
        // ensures that the ROM at least has one bank worth of data.
        if (address < size)
          return data[address];
        return 0xFF;
      // ROM bank N
      case 0x4 ... 0x7: {
        auto effective_address = (rom_bank << 14) | (address & 0x3FFF);
        if (effective_address < size)
          return data[effective_address];
        return 0xFF;
      }
      // SRAM bank
      case 0xA ... 0xB:
        return sram->Read((ram_bank << 13) | (address & 0x1FFF));
    }
  }

  void Write(std::uint16_t address, std::uint8_t value) override {
    switch (address >> 12) {
      // ROM Bank Number
      case 0x2 ... 0x3:
        rom_bank = value & 0x7F;
        if (rom_bank == 0)
          rom_bank = 1;
        break;
      // RAM Bank Number
      case 0x4 ... 0x5:
        ram_bank = value & 3;
        break;
      // SRAM bank
      case 0xA ... 0xB:
        sram->Write((ram_bank << 13) | (address & 0x1FFF), value);
        break;
    }
  }

private:
  std::unique_ptr<std::uint8_t[]> data;
  std::unique_ptr<BackupFile> sram;
  size_t size;

  std::uint8_t rom_bank = 1;
  std::uint8_t ram_bank = 0;
};
