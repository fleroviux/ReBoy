/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdio>
#include <fstream>
#include <string>
#include <memory>

#include "apu/apu.hpp"
#include "cpu/cpu.hpp"
#include "irq.hpp"
#include "memory.hpp"
#include "ppu/ppu.hpp"
#include "mbc/no_mbc.hpp"
#include "mbc/mbc3.hpp"
#include "joypad.hpp"
#include "scheduler.hpp"
#include "timer.hpp"

class GameBoy {
public:
  GameBoy() :
    irq(&cpu),
    ppu(&scheduler, &irq),
    apu(&scheduler),
    timer(&scheduler, &irq),
    memory(&scheduler, &irq, &ppu, &apu, &timer, &joypad),
    cpu(&memory) { Reset(); }

  void Reset() {
    scheduler.Reset();
    irq.Reset();
    ppu.Reset();
    apu.Reset();
    timer.Reset();
    joypad.Reset();
    memory.Reset();
    cpu.Reset();
  }

  auto GetJoypad() -> Joypad& { return joypad; }

  void SetAudioDevice(AudioDevice* device) {
    apu.SetAudioDevice(device);
  }

  bool LoadBootROM(std::string const& path) {
    size_t size;
    std::ifstream file {path, std::ios::binary | std::ios::in};
    if (!file.good()) {
      std::printf("Cannot open %s, please provide a BOOTROM dump.\n", path.c_str());
      return false;
    }
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0);
    if (size != 256 && size != 2304) {
      std::puts("Bad BOOTROM dump: file must be exactly 256 (GB) or 2304 (GBC) bytes big. ");
      return false;
    }
    file.read((char*)memory.boot, 256);
    if (size == 2304) {
      file.seekg(0x200);
      file.read((char *) memory.boot_cgb, 0x700);
      memory.enable_is_cgb = true;
    } else {
      memory.enable_is_cgb = false;
    }
    return true;
  }

  bool LoadGame(std::string const& path) {
    // TODO: more validation, e.g. based on the MBC type.
    std::ifstream file{path, std::ios::in | std::ios::binary};

    if (!file.good()) {
      std::printf("Failed to open ROM: %s\n", path.c_str());
      return false;
    }

    size_t size;
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0);

    if ((size & 0x3FFF) != 0) {
      std::puts("ROM size must be multiple of 16 KiB");
      return false;
    }

    if (size == 0) {
      std::puts("ROM may not be empty");
      return false;
    }

    if ((size >> 14) > 256) {
      std::puts("ROM may not be bigger than 4 MIB");
      return false;
    }

    auto data = std::make_unique<std::uint8_t[]>(size);
    file.read((char*)data.get(), size);

    // FIXME: remove original file extension.
    auto save_path = path + ".sav";

    // Create mapper depending on header value at 0x147
    switch (data[0x147]) {
      // ROM (+ RAM (+ BATTERY))
      case 0x00:
      case 0x08:
      case 0x09:
        mapper = std::make_unique<NoMBC>(std::move(data), size);
        break;

      // TODO: we are treating MBC1 like MBC3 for now.
      // This isn't really accurate but good enough for now.
      case 0x01:
      case 0x03:
        std::puts("Warning: unimplemented MBC1 mapper, using MBC3 instead.");
      // MBC3
      case 0x0F ... 0x13:
        mapper = std::make_unique<MBC3>(std::move(data), size, save_path);
        break;
      default: {
        mapper.reset();
        std::printf("Bad or unknown mapper 0x%02X", data[0x147]);
        break;
      }
    }

    memory.mapper = mapper.get();
    return true;
  }

  void Frame(std::uint32_t* buffer) {
    auto target = scheduler.GetTimestampNow() + 70224;

    ppu.SetBuffer(buffer);

    while (scheduler.GetTimestampNow() < target) {
      if (cpu.IsHalted()) {
        // TODO: fast skip to the next event?
        scheduler.AddCycles(4);
        scheduler.Step();
        apu.Step(); // weird but does the job.
      } else {
        cpu.Step();
      }
      irq.Step();
    }
  }

private:
  Scheduler scheduler;
  IRQ irq;
  PPU ppu;
  APU apu;
  Timer timer;
  Joypad joypad;
  Memory memory;
  CPU cpu;
  std::unique_ptr<MBCBase> mapper;
};
