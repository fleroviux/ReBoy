/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "joypad.hpp"

void Joypad::Reset() {
  keystate = 0xFF;
  select_button_keys = false;
  select_direction_keys = false;
}

auto Joypad::Read() -> std::uint8_t {
  std::uint8_t value = 0;
  if (select_button_keys)
    value |= (keystate >> 2) & 3;
  else
    value |= keystate & 3;
  if (select_direction_keys)
    value |= (keystate >> 2) & 0xC;
  else
    value |= (keystate >> 4) & 0xC;

  value |= select_button_keys ? (1 << 5) : 0;
  value |= select_direction_keys ? (1 << 4) : 0;
  return value;
}

void Joypad::Write(std::uint8_t value) {
  select_button_keys = value & (1 << 5);
  select_direction_keys = value & (1 << 4);
}

void Joypad::SetKeyState(Key key, bool pressed) {
  if (pressed)
    keystate &= ~(static_cast<std::uint8_t>(key));
  else
    keystate |= static_cast<std::uint8_t>(key);
}
