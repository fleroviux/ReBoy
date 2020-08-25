/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

#include "sequencer.hpp"

class WaveChannel {
public:
  WaveChannel(Scheduler* scheduler);

  void Reset();

  void Generate(int cycles_late);
  auto Read (int offset) -> std::uint8_t;
  void Write(int offset, std::uint8_t value);

  auto ReadSample(int offset) -> std::uint8_t {
    return wave_ram[0][offset];
  }

  void WriteSample(int offset, std::uint8_t value) {
    wave_ram[0][offset] = value;
  }

  std::int8_t sample = 0;

private:
  constexpr int GetSynthesisIntervalFromFrequency(int frequency) {
    // 2 cycles equals 2097152 Hz, the highest possible sample rate.
    return 2 * (2048 - frequency);
  }

  std::function<void(int)> event_cb = [this](int cycles_late) {
    this->Generate(cycles_late);
  };

  Scheduler* scheduler;
  Sequencer sequencer;

  bool enabled;
  bool force_volume;
  int  volume;
  int  frequency;
  int  dimension;
  int  wave_bank;
  bool length_enable;

  std::uint8_t wave_ram[2][16];

  int phase;
};
