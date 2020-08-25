/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <mutex>

#include "channel/channel_noise.hpp"
#include "channel/channel_quad.hpp"
#include "channel/channel_wave.hpp"
#include "../../common/dsp/resampler.hpp"
#include "../../common/dsp/ring_buffer.hpp"
#include "../../device/audio_device.hpp"

class APU {
public:
  APU(Scheduler* scheduler);

  void Reset();
  void SetAudioDevice(AudioDevice* device);
  void Step();
  auto ReadMMIO(std::uint8_t reg) -> std::uint8_t;
  void WriteMMIO(std::uint8_t reg, std::uint8_t value);

private:
  friend void AudioCallback(APU* apu, std::int16_t* stream, int byte_len);

  enum Registers {
    // Sound Channel 1 - Tone & Sweep
    REG_NR10 = 0x10,
    REG_NR11 = 0x11,
    REG_NR12 = 0x12,
    REG_NR13 = 0x13,
    REG_NR14 = 0x14,

    // Sound Channel 2 - Tone
    REG_NR21 = 0x16,
    REG_NR22 = 0x17,
    REG_NR23 = 0x18,
    REG_NR24 = 0x19,

    // Sound Channel 3 - Wave Output
    REG_NR30 = 0x1A,
    REG_NR31 = 0x1B,
    REG_NR32 = 0x1C,
    REG_NR33 = 0x1D,
    REG_NR34 = 0x1E,
    REG_WAVERAM = 0x30,

    // Sound Channel 4 - Noise
    REG_NR41 = 0x20,
    REG_NR42 = 0x21,
    REG_NR43 = 0x22,
    REG_NR44 = 0x23
  };

  QuadChannel psg1;
  QuadChannel psg2;
  WaveChannel psg3;
  NoiseChannel psg4;

  int frequency_divider;
  float averaged_sample;

  std::mutex buffer_mutex;
  std::shared_ptr<common::dsp::StereoRingBuffer<float>> buffer;
  std::unique_ptr<common::dsp::StereoResampler<float>> resampler;

  Scheduler* scheduler;
  AudioDevice* audio_device;
  NullAudioDevice null_audio_device;
};
