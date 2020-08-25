/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "channel_quad.hpp"

QuadChannel::QuadChannel(Scheduler* scheduler) : scheduler(scheduler), sequencer(scheduler) {
  sequencer.sweep.enabled = true;
  sequencer.envelope.enabled = true;
  Reset();
}

void QuadChannel::Reset() {
  sequencer.Reset();
  phase = 0;
  sample = 0;
  wave_duty = 0;
  length_enable = false;
  scheduler->Add(GetSynthesisIntervalFromFrequency(0), event_cb);
}

void QuadChannel::Generate(int cycles_late) {
  if ((length_enable && sequencer.length <= 0) || sequencer.sweep.channel_disabled) {
    sample = 0;
    scheduler->Add(GetSynthesisIntervalFromFrequency(0) - cycles_late, event_cb);
    return;
  }

  constexpr std::int16_t pattern[4][8] = {
          { +8, -8, -8, -8, -8, -8, -8, -8 },
          { +8, +8, -8, -8, -8, -8, -8, -8 },
          { +8, +8, +8, +8, -8, -8, -8, -8 },
          { +8, +8, +8, +8, +8, +8, -8, -8 }
  };

  sample = std::int8_t(pattern[wave_duty][phase] * sequencer.envelope.current_volume);
  phase = (phase + 1) % 8;

  scheduler->Add(GetSynthesisIntervalFromFrequency(sequencer.sweep.current_freq) - cycles_late, event_cb);
}

auto QuadChannel::Read(int offset) -> std::uint8_t {
  auto& sweep = sequencer.sweep;
  auto& envelope = sequencer.envelope;

  switch (offset) {
    /* Sweep Register */
    case 0: {
      return sweep.shift |
             ((int)sweep.direction << 3) |
             (sweep.divider << 4);
    }

    /* Wave Duty / Length / Envelope */
    case 1: {
      return wave_duty << 6;
    }
    case 2: {
      return envelope.divider |
             ((int)envelope.direction << 3) |
             (envelope.initial_volume << 4);
    }

    /* Frequency / Control */
    case 4: {
      return length_enable ? 0x40 : 0;
    }
  }

  return 0;
}

void QuadChannel::Write(int offset, std::uint8_t value) {
  auto& sweep = sequencer.sweep;
  auto& envelope = sequencer.envelope;

  switch (offset) {
    /* Sweep Register */
    case 0: {
      sweep.shift = value & 7;
      sweep.direction = Sweep::Direction((value >> 3) & 1);
      sweep.divider = (value >> 4) & 7;
      break;
    }

    /* Wave Duty / Length / Envelope */
    case 1: {
      sequencer.length = 64 - (value & 63);
      wave_duty = (value >> 6) & 3;
      break;
    }
    case 2: {
      envelope.divider = value & 7;
      envelope.direction = Envelope::Direction((value >> 3) & 1);
      envelope.initial_volume = value >> 4;
      break;
    }

    /* Frequency / Control */
    case 3: {
      sweep.initial_freq = (sweep.initial_freq & ~0xFF) | value;
      sweep.current_freq = sweep.initial_freq;
      break;
    }
    case 4: {
      sweep.initial_freq = (sweep.initial_freq & 0xFF) | (((int)value & 7) << 8);
      sweep.current_freq = sweep.initial_freq;
      length_enable = value & 0x40;

      if (value & 0x80) {
        phase = 0;
        sequencer.Restart();
      }

      break;
    }
  }
}
