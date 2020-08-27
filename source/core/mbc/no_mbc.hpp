/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <memory>

#include "mbc.hpp"

class NoMBC : public MBCBase {
public:
  NoMBC(std::unique_ptr<std::uint8_t[]> data, size_t size)
    : data(std::move(data)), size(size) { }

  auto Read(std::uint16_t address) -> std::uint8_t override {
    switch (address >> 12) {
      case 0 ... 7:
        if (address < size)
          return data[address];
        return 0xFF;
    }
  }

  void Write(std::uint16_t address, std::uint8_t value) override {
    // ...
  }
private:
  std::unique_ptr<std::uint8_t[]> data;
  size_t size;
};
