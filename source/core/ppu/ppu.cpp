/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include <cstring>
#include <cstdio>

#include "ppu.hpp"

constexpr std::uint32_t PPU::kColorPalette[4];

PPU::PPU(Scheduler* scheduler, IRQ* irq) : scheduler(scheduler), irq(irq)  {
  Reset();
}

void PPU::Reset() {
  std::memset(vram, 0, 0x4000);
  std::memset(oam, 0, 0xA0);
  lcdc = {};
  stat = {};
  scy = 0;
  scx = 0;
  bgp = 0;
  obp[0] = 0;
  obp[1] = 0;
  ly = 0;
  lyc = 0;
  hblank_irq_flag_old = false;
  vblank_irq_flag_old = false;
  vcount_irq_flag_old = false;
  stat.mode = Mode::Search;
  Schedule(Mode::Transfer, 0);
  oam_is_dirty = true;
  SearchAndPrioritizeOBJs();
}

void PPU::RenderScanline() {
  if (buffer == nullptr)
    return;

  for (int x = 0; x < 160; x++)
    bg_is_color0[x] = true;

  if (lcdc.enable_bg)
    RenderBackground();

  if (lcdc.enable_win && ly >= wy)
    RenderWindow();

  if (lcdc.enable_obj)
    RenderSprites();
}

void PPU::RenderBackground() {
  auto line = &buffer[160 * ly];

  auto map_y = (ly + scy) & 0xFF;
  auto block_x = scx >> 3;
  auto block_y = map_y >> 3;
  auto tile_y = map_y & 7;
  auto map_data = &vram[0x1800 + 0x400 * lcdc.bg_map_select + block_y * 32];

  auto screen_x = -(scx & 7);

  while (screen_x < 160) {
    auto index = 0;
    auto tile = map_data[block_x];
    std::uint8_t byte0, byte1;

    if (lcdc.bg_win_tile_select == 1) {
      index = tile << 4 | tile_y << 1;
    } else {
      index = 0x1000 + std::int8_t(tile) * 16 + tile_y * 2;
    }

    byte0 = vram[index | 0];
    byte1 = vram[index | 1];

    for (int tile_x = 0; tile_x < 8; tile_x++) {
      auto palette_index = (byte0 >> 7) | ((byte1 >> 7) << 1);
      byte0 <<= 1;
      byte1 <<= 1;
      if (screen_x >= 0 && screen_x < 160) {
        line[screen_x] = kColorPalette[(bgp >> (palette_index * 2)) & 3];
        bg_is_color0[screen_x] = palette_index == 0;
      }
      screen_x++;
    }

    block_x = (block_x + 1) & 0x1F;
  }
}

void PPU::RenderWindow() {
  auto line = &buffer[160 * ly];

  auto map_y = ly - wy;
  auto tile_y = map_y & 7;
  auto block_y = map_y >> 3;
  auto map_data = &vram[0x1800 + 0x400 * lcdc.win_map_select + block_y * 32];

  auto screen_x = wx - 7;
  auto block_x = 0;

  while (screen_x < 160) {
    auto index = 0;
    auto tile = map_data[block_x++];
    std::uint8_t byte0, byte1;

    if (lcdc.bg_win_tile_select == 1) {
      index = tile << 4 | tile_y << 1;
    } else {
      index = 0x1000 + std::int8_t(tile) * 16 + tile_y * 2;
    }

    byte0 = vram[index | 0];
    byte1 = vram[index | 1];

    for (int tile_x = 0; tile_x < 8; tile_x++) {
      auto palette_index = (byte0 >> 7) | ((byte1 >> 7) << 1);
      byte0 <<= 1;
      byte1 <<= 1;
      if (screen_x >= 0 && screen_x < 160) {
        line[screen_x] = kColorPalette[(bgp >> (palette_index * 2)) & 3];
        bg_is_color0[screen_x] = palette_index == 0;
      }
      screen_x++;
    }
  }
}

void PPU::RenderSprites() {
  auto line = &buffer[160 * ly];

  auto const& sorted = sorted_objs[ly];

  for (int i = sorted.count - 1; i >= 0; i--) {
    auto sprite = sorted.list[i];
    auto x = int(sprite->x) - 8;
    auto y = int(sprite->y) - 16;
    auto tile = sprite->tile;
    auto tile_y = ly - y;
    if (sprite->flip_y) {
      tile_y ^= lcdc.obj_double_size ? 15 : 7;
    }
    if (lcdc.obj_double_size) {
      tile = (tile & ~1) | (tile_y >> 3);
      tile_y &= 7;
    }
    auto index = (tile << 4) | (tile_y << 1);
    auto byte0 = vram[index | 0];
    auto byte1 = vram[index | 1];
    auto x_xor = sprite->flip_x ? 7 : 0;
    auto pal = obp[sprite->palette];
    for (int tile_x = 0; tile_x < 8; tile_x++) {
      auto palette_index = (byte0 >> 7) | ((byte1 >> 7) << 1);
      byte0 <<= 1;
      byte1 <<= 1;
      if (palette_index == 0) {
        continue;
      }
      auto screen_x = x + (tile_x ^ x_xor);
      if (screen_x >= 0 && screen_x < 160 && (!sprite->behind_bg || bg_is_color0[screen_x])) {
        line[screen_x] = kColorPalette[(pal >> (palette_index * 2)) & 3];
      }
    }
  }
}

void PPU::SearchAndPrioritizeOBJs() {
  if (!oam_is_dirty) {
    return;
  }

  oam_is_dirty = false;

  auto height = lcdc.obj_double_size ? 16 : 8;

  for (int line = 0; line < 144; line++) {
    for (auto& bucket : buckets) {
      bucket = {};
    }

    for (auto const& sprite : objs) {
      auto y = int(sprite.y) - 16;
      if (line < y || line >= (y + height)) {
        continue;
      }
      auto& bucket = buckets[sprite.x];
      if (bucket.count == 10) {
        continue;
      }
      bucket.list[bucket.count++] = &sprite;
    }

    // Collect at maximum 10 sprites from the buckets.
    int i = 0;
    int j = 0;
    auto& sorted = sorted_objs[line];
    sorted = {};
    while (sorted.count < 10 && i < 256) {
      auto const& bucket = buckets[i];
      if (j == bucket.count) {
        i++;
        j = 0;
        continue;
      }
      sorted.list[sorted.count++] = bucket.list[j++];
    }
  }
}

void PPU::CheckSTATInterrupt() {
  stat.coincidence_flag = ly == lyc;

  auto hblank_irq_flag = stat.hblank_irq && stat.mode == Mode::HBlank;
  auto vblank_irq_flag = stat.vblank_irq && stat.mode == Mode::VBlank;
  auto vcount_irq_flag = stat.coincidence_irq && stat.coincidence_flag;

  if ((!hblank_irq_flag_old && hblank_irq_flag) ||
      (!vblank_irq_flag_old && vblank_irq_flag) ||
      (!vcount_irq_flag_old && vcount_irq_flag))
    irq->Raise(IRQ::LCD_STAT);

  hblank_irq_flag_old = hblank_irq_flag;
  vblank_irq_flag_old = vblank_irq_flag;
  vcount_irq_flag_old = vcount_irq_flag;
}

void PPU::Schedule(Mode mode, int cycles_late) {
  stat.mode = mode;
  CheckSTATInterrupt();
  switch (mode) {
    case Mode::HBlank:
      scheduler->Add(204 - cycles_late, [this](int cycles_late) {
        if (++ly == 144) {
          Schedule(Mode::VBlank, cycles_late);
          irq->Raise(IRQ::VBLANK);
        } else {
          Schedule(Mode::Search, cycles_late);
          SearchAndPrioritizeOBJs();
        }
      });
      break;
    case Mode::VBlank:
      scheduler->Add(456 - cycles_late, [this](int cycles_late) {
        if (++ly == 154) {
          ly = 0;
          Schedule(Mode::Search, cycles_late);
          SearchAndPrioritizeOBJs();
        } else {
          Schedule(Mode::VBlank, cycles_late);
        }
      });
      break;
    case Mode::Search:
      scheduler->Add(80 - cycles_late, [this](int cycles_late) {
        Schedule(Mode::Transfer, cycles_late);
      });
      break;
    case Mode::Transfer:
      scheduler->Add(172 - cycles_late, [this](int cycles_late) {
        // Drawing scanline at the end of the "transfer" period.
        RenderScanline();
        Schedule(Mode::HBlank, cycles_late);
      });
      break;
  }
}

auto PPU::ReadMMIO(std::uint8_t reg) -> std::uint8_t {
  switch (reg) {
    case REG_LCDC:
      return (lcdc.enable_bg ? 1 : 0) |
             (lcdc.enable_obj ? 2 : 0) |
             (lcdc.obj_double_size ? 4 : 0) |
             (lcdc.bg_map_select << 3) |
             (lcdc.bg_win_tile_select << 4) |
             (lcdc.enable_win ? 32 : 0) |
             (lcdc.win_map_select << 6) |
             (lcdc.enable_display ? 128 : 0);
    case REG_STAT:
      return static_cast<int>(stat.mode) |
             (stat.coincidence_flag ? 4 : 0) |
             (stat.hblank_irq ?  8 : 0) |
             (stat.vblank_irq ? 16 : 0) |
             (stat.search_irq ? 32 : 0) |
             (stat.coincidence_irq ? 64 : 0);
    case REG_SCY:
      return scy;
    case REG_SCX:
      return scx;
    case REG_LY:
      return ly;
    case REG_LYC:
      return lyc;
    case REG_BGP:
      return bgp;
    case REG_OBP0:
      return obp[0];
    case REG_OBP1:
      return obp[1];
    case REG_WY:
      return wy;
    case REG_WX:
      return wx;
    default:
      std::printf("Unhandled PPU IO read from 0xFF%02X\n", reg);
      return 0;
  }
}

void PPU::WriteMMIO(std::uint8_t reg, std::uint8_t value) {
  switch (reg) {
    case REG_LCDC:
      lcdc.enable_bg = value & 1;
      lcdc.enable_obj = value & 2;
      lcdc.obj_double_size = value & 4;
      lcdc.bg_map_select = (value >> 3) & 1;
      lcdc.bg_win_tile_select = (value >> 4) & 1;
      lcdc.enable_win = value & 32;
      lcdc.win_map_select = (value >> 6) & 1;
      lcdc.enable_display = value & 128;
      break;
    case REG_STAT:
      stat.hblank_irq = value & 8;
      stat.vblank_irq = value & 16;
      stat.search_irq = value & 32;
      stat.coincidence_irq = value & 64;
      CheckSTATInterrupt();
      break;
    case REG_SCY:
      scy = value;
      break;
    case REG_SCX:
      scx = value;
      break;
    case REG_LYC:
      lyc = value;
      CheckSTATInterrupt();
      break;
    case REG_BGP:
      bgp = value;
      break;
    case REG_OBP0:
      obp[0] = value;
      break;
    case REG_OBP1:
      obp[1] = value;
      break;
    case REG_WY:
      wy = value;
      break;
    case REG_WX:
      wx = value;
      break;
    default:
      std::printf("Unhandled PPU IO write to 0xFF%02X = 0x%02X\n", reg, value);
  }
}
