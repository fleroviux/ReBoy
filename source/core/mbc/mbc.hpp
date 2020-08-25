/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

class MBCBase {
public:
  virtual ~MBCBase() = default;

  virtual auto Read(std::uint16_t address) -> std::uint8_t = 0;
  virtual void Write(std::uint16_t address, std::uint8_t value) = 0;
  virtual auto GetROM1Bank() -> std::uint8_t { return 1; }
};
