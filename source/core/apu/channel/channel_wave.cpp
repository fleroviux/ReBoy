/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "channel_wave.hpp"

WaveChannel::WaveChannel(Scheduler* scheduler) : scheduler(scheduler), sequencer(scheduler) {
  sequencer.sweep.enabled = false;
  sequencer.envelope.enabled = false;
  sequencer.length_default = 256;
  Reset();
}

void WaveChannel::Reset() {
  sequencer.Reset();

  phase = 0;
  sample = 0;

  enabled = false;
  force_volume = false;
  volume = 0;
  frequency = 0;
  dimension = 0;
  wave_bank = 0;
  length_enable = false;

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 16; j++) {
      wave_ram[i][j] = 0;
    }
  }

  scheduler->Add(GetSynthesisIntervalFromFrequency(0), event_cb);
}

void WaveChannel::Generate(int cycles_late) {
  if (!enabled || (length_enable && sequencer.length <= 0)) {
    sample = 0;
    scheduler->Add(GetSynthesisIntervalFromFrequency(0) - cycles_late, event_cb);
    return;
  }

  auto byte = wave_ram[0][phase / 2];

  if ((phase % 2) == 0) {
    sample = byte >> 4;
  } else {
    sample = byte & 15;
  }

  constexpr int volume_table[4] = { 0, 4, 2, 1 };

  /* TODO: at 100% sample might overflow. */
  sample = (sample - 8) * 4 * (force_volume ? 3 : volume_table[volume]);

  if (++phase == 32) {
    phase = 0;
  }

  scheduler->Add(GetSynthesisIntervalFromFrequency(frequency) - cycles_late, event_cb);
}

auto WaveChannel::Read(int offset) -> std::uint8_t {
  switch (offset) {
    /* Stop / Wave RAM select */
    case 0: {
      return (dimension << 5) |
             (wave_bank << 6) |
             (enabled ? 0x80 : 0);
    }

    /* Length / Volume */
    case 1: return 0;
    case 2: {
      return (volume << 5) |
             (force_volume  ? 0x80 : 0);
    }

    /* Frequency / Control */
    case 3: return 0;
    case 4: {
      return length_enable ? 0x40 : 0;
    }

    default: return 0;
  }
}

void WaveChannel::Write(int offset, std::uint8_t value) {
  switch (offset) {
    /* Stop / Wave RAM select */
    case 0: {
      enabled = value & 0x80;
      break;
    }

    /* Length / Volume */
    case 1: {
      sequencer.length = 256 - value;
      break;
    }
    case 2: {
      volume = (value >> 5) & 3;
      force_volume = value & 0x80;
      break;
    }

    /* Frequency / Control */
    case 3: {
      frequency = (frequency & ~0xFF) | value;
      break;
    }
    case 4: {
      frequency = (frequency & 0xFF) | ((value & 7) << 8);
      length_enable = value & 0x40;

      if (value & 0x80) {
        phase = 0;
        sequencer.Restart();
      }
      break;
    }
  }
}
