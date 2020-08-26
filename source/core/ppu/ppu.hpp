/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

#include "../irq.hpp"
#include "../scheduler.hpp"

class PPU {
public:
  PPU(Scheduler* scheduler, IRQ* irq);

  void Reset();

  void SetBuffer(std::uint32_t* buffer) {
    this->buffer = buffer;
  }

  auto ReadVRAM(std::uint16_t offset) -> std::uint8_t {
    return vram[offset];
  }

  void WriteVRAM(std::uint16_t offset, std::uint8_t value) {
    vram[offset] = value;
  }

  auto ReadOAM(std::uint8_t offset) -> std::uint8_t {
    return oam[offset];
  }

  void WriteOAM(std::uint8_t offset, std::uint8_t value) {
    oam[offset] = value;

    // Decode sprite data into our convenient structure.
    auto& sprite = objs[offset >> 2];
    switch (offset & 3) {
      case 0:
        if (sprite.y != value)
          oam_is_dirty = true;
        sprite.y = value;
        break;
      case 1:
        if (sprite.x != value)
          oam_is_dirty = true;
        sprite.x = value;
        break;
      case 2:
        sprite.tile = value;
        break;
      case 3:
        sprite.palette = (value >> 4) & 1;
        sprite.flip_x = value & (1 << 5);
        sprite.flip_y = value & (1 << 6);
        sprite.behind_bg = value & (1 << 7);
        break;
    }
  }

  auto ReadMMIO(std::uint8_t reg) -> std::uint8_t;
  void WriteMMIO(std::uint8_t reg, std::uint8_t value);

private:
  enum Registers {
    REG_LCDC = 0x40,
    REG_STAT = 0x41,
    REG_SCY = 0x42,
    REG_SCX = 0x43,
    REG_LY = 0x44,
    REG_LYC = 0x45,
    REG_BGP = 0x47,
    REG_OBP0 = 0x48,
    REG_OBP1 = 0x49,
    REG_WY = 0x4A,
    REG_WX = 0x4B
  };

  enum class Mode {
    HBlank = 0,
    VBlank = 1,
    Search = 2,
    Transfer = 3
  };

  std::uint8_t vram[0x4000];
  std::uint8_t oam[0xA0];

  struct LCDC {
    bool enable_bg = false;
    bool enable_obj = false;
    bool obj_double_size = false;
    int bg_map_select = 0;
    int bg_win_tile_select = 0;
    bool enable_win = false;
    int win_map_select = 0;
    bool enable_display = false;
  } lcdc;

  struct STAT {
    Mode mode = Mode::Search;
    bool coincidence_flag = false;
    bool hblank_irq = false;
    bool vblank_irq = false;
    bool search_irq = false;
    bool coincidence_irq = false;
  } stat;

  std::uint8_t scy;
  std::uint8_t scx;
  std::uint8_t bgp;
  std::uint8_t obp[2];
  std::uint8_t ly;
  std::uint8_t lyc;
  std::uint8_t wy;
  std::uint8_t wx;

  bool bg_is_color0[160];

  /// Indicates that OBJs changed and need to be sorted again.
  bool oam_is_dirty;

  /// List of all current OBJs with pre-decoded information.
  struct OAM {
    std::uint8_t x;
    std::uint8_t y;
    std::uint8_t tile;
    int palette;
    bool flip_x;
    bool flip_y;
    bool behind_bg;
  } objs[40];

  /// Buckets for bucketsort, used to prioritze OBJs.
  struct OAMSearchBucket {
    int count = 0;
    OAM const* list[10];
  } buckets[256];

  /// List up to 10 OBJs which will be rendered for each scanline.
  /// Sorted by priority in ascending order.
  struct OAMSortedList {
    int count = {};
    OAM const* list[10];
  } sorted_objs[144];

  Scheduler* scheduler;
  IRQ* irq;
  bool hblank_irq_flag_old;
  bool vblank_irq_flag_old;
  bool vcount_irq_flag_old;
  std::uint32_t* buffer;

  void RenderScanline();
  void RenderBackground();
  void RenderWindow();
  void RenderSprites();
  void SearchAndPrioritizeOBJs();
  void CheckSTATInterrupt();
  void Schedule(Mode mode, int cycles_late);

  static constexpr std::uint32_t kColorPalette[4] = {
    0xFFFFFFFF, 0xFF606060, 0xFF202020, 0xFF000000 };
};
