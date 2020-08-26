/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "SDL.h"
#undef main

#include "audio_device.hpp"
#include "../../core/gameboy.hpp"

static std::uint32_t g_buffer[160 * 144];

void usage(const char* name) {
  std::printf("%s rom_path.gb\n", name);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    usage(argc == 0 ? nullptr : argv[0]);
    return -1;
  }

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  auto window = SDL_CreateWindow("ReBoy",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    640,
    576,
    0);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
  auto event = SDL_Event{};

  SDL_RenderSetLogicalSize(renderer, 160, 144);

  auto gameboy = new GameBoy();

  if (!gameboy->LoadBootROM("boot_cgb.bin")) {
    return -2;
  }

  if (!gameboy->LoadGame(std::string{argv[1]})) {
    return -3;
  }

  auto audio_device = new SDL2_AudioDevice();
  gameboy->SetAudioDevice(audio_device);

  auto frames = 0;
  auto time_start = SDL_GetTicks();

  SDL_GL_SetSwapInterval(1);

  while (true) {
    gameboy->Frame(g_buffer);
    frames++;

    auto time_now = SDL_GetTicks();
    if ((time_now - time_start) >= 1000) {
      auto percentage = int(frames / 60.0 * 100.0);
      auto window_title = "ReBoy [" + std::to_string(percentage) + "% | " + std::to_string(frames) + " fps]";
      SDL_SetWindowTitle(window, window_title.c_str());
      frames = 0;
      time_start = SDL_GetTicks();
    }

    SDL_UpdateTexture(texture, nullptr, g_buffer, sizeof(std::uint32_t) * 160);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto done;
      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool pressed = event.type == SDL_KEYDOWN;
        auto key_event = reinterpret_cast<SDL_KeyboardEvent*>(&event);
        switch (key_event->keysym.sym) {
          case SDLK_a:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::A, pressed);
            break;
          case SDLK_s:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::B, pressed);
            break;
          case SDLK_UP:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Up, pressed);
            break;
          case SDLK_DOWN:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Down, pressed);
            break;
          case SDLK_LEFT:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Left, pressed);
            break;
          case SDLK_RIGHT:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Right, pressed);
            break;
          case SDLK_BACKSLASH:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Select, pressed);
            break;
          case SDLK_RETURN:
            gameboy->GetJoypad().SetKeyState(Joypad::Key::Start, pressed);
            break;
          case SDLK_SPACE:
            SDL_GL_SetSwapInterval(pressed ? 0 : 1);
            break;
        }
      }
    }
  }

done:
  delete audio_device;
  delete gameboy;
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  return 0;
}
