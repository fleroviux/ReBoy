/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "apu.hpp"
#include "../../common/dsp/resampler/windowed-sinc.hpp"

/* Implemented in callback.cpp */
void AudioCallback(APU* apu, std::int16_t* stream, int byte_len);

APU::APU(Scheduler* scheduler)
  : psg1(scheduler)
  , psg2(scheduler)
  , psg3(scheduler)
  , psg4(scheduler)
{
  Reset();
}

void APU::Reset() {
  psg1.Reset();
  psg2.Reset();
  psg3.Reset();
  psg4.Reset();
  SetAudioDevice(&null_audio_device);
  frequency_divider = 0;
  averaged_sample = 0.0f;
}

void APU::Step() {
  averaged_sample += (psg1.sample / 128.0f + psg2.sample / 128.0f + psg3.sample / 128.0f + psg4.sample / 128.0f) * 0.25f;
  if (++frequency_divider == 16) {
    averaged_sample *= 1.0 / 16.0;
    frequency_divider = 0;
    std::lock_guard guard{buffer_mutex};
    resampler->Write({ averaged_sample, averaged_sample });
    averaged_sample = 0.0f;
  }
}

void APU::SetAudioDevice(AudioDevice* device) {
  if (device == nullptr)
    device = &null_audio_device;
  if (audio_device != nullptr)
    audio_device->Close();
  audio_device = device;
  // TODO: handle error when opening audio device.
  audio_device->Open(this, (AudioDevice::Callback)AudioCallback);
  // TODO: this is a potential race-condition. Explicitly unpause audio device *after* Open() call?
  buffer = std::make_shared<common::dsp::StereoRingBuffer<float>>(audio_device->GetBlockSize() * 4, true);
  resampler = std::make_unique<common::dsp::SincStereoResampler<float, 32>>(buffer);
  resampler->SetSampleRates(65536, audio_device->GetSampleRate());
}

auto APU::ReadMMIO(std::uint8_t reg) -> std::uint8_t {
  switch (reg) {
    // Sound Channel 1 - Tone & Sweep
    case REG_NR10:
      return psg1.Read(0);
    case REG_NR11:
      return psg1.Read(1);
    case REG_NR12:
      return psg1.Read(2);
    case REG_NR14:
      return psg1.Read(4);

    // Sound Channel 2 - Tone
    case REG_NR21:
      return psg2.Read(1);
    case REG_NR22:
      return psg2.Read(2);
    case REG_NR24:
      return psg2.Read(4);

    // Sound Channel 3 - Wave Output
    case REG_NR30:
      return psg3.Read(0);
    case REG_NR31:
      return psg3.Read(1);
    case REG_NR32:
      return psg3.Read(2);
    case REG_NR34:
      return psg3.Read(4);
    case REG_WAVERAM|0x0 ... REG_WAVERAM|0xF:
      return psg3.ReadSample(reg - REG_WAVERAM);

    // Sound Channel 4 - Noise
    case REG_NR41:
      return psg4.Read(0);
    case REG_NR42:
      return psg4.Read(1);
    case REG_NR43:
      return psg4.Read(2);
    case REG_NR44:
      return psg4.Read(3);
  }

  return 0;
}

void APU::WriteMMIO(std::uint8_t reg, std::uint8_t value) {
  switch (reg) {
    // Sound Channel 1 - Tone & Sweep
    case REG_NR10:
      psg1.Write(0, value);
      break;
    case REG_NR11:
      psg1.Write(1, value);
      break;
    case REG_NR12:
      psg1.Write(2, value);
      break;
    case REG_NR13:
      psg1.Write(3, value);
      break;
    case REG_NR14:
      psg1.Write(4, value);
      break;

    // Sound Channel 2 - Tone
    case REG_NR21:
      psg2.Write(1, value);
      break;
    case REG_NR22:
      psg2.Write(2, value);
      break;
    case REG_NR23:
      psg2.Write(3, value);
      break;
    case REG_NR24:
      psg2.Write(4, value);
      break;

    // Sound Channel 3 - Wave Output
    case REG_NR30:
      psg3.Write(0, value);
      break;
    case REG_NR31:
      psg3.Write(1, value);
      break;
    case REG_NR32:
      psg3.Write(2, value);
      break;
    case REG_NR33:
      psg3.Write(3, value);
      break;
    case REG_NR34:
      psg3.Write(4, value);
      break;
    case (REG_WAVERAM|0x0) ... (REG_WAVERAM|0xF):
      psg3.WriteSample(reg - REG_WAVERAM, value);
      break;

    // Sound Channel 4 - Noise
    case REG_NR41:
      psg4.Write(0, value);
      break;
    case REG_NR42:
      psg4.Write(1, value);
      break;
    case REG_NR43:
      psg4.Write(2, value);
      break;
    case REG_NR44:
      psg4.Write(3, value);
      break;
  }
}
