/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

class Joypad {
public:
  Joypad() { Reset(); }

  enum class Key {
    A = 1,
    B = 2,
    Right = 4,
    Left = 8,
    Select = 16,
    Start = 32,
    Up = 64,
    Down = 128
  };

  void Reset();
  auto Read() -> std::uint8_t;
  void Write(std::uint8_t value);
  void SetKeyState(Key key, bool pressed);

private:
  std::uint8_t keystate;
  bool select_button_keys;
  bool select_direction_keys;
};
