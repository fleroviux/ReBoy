cmake_minimum_required(VERSION 3.16)
project(GBemu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(FindSDL2)
find_package(SDL2 REQUIRED)

add_executable(ReBoy
        source/core/cpu/cpu.hpp
        source/core/cpu/cpu.cpp
        source/core/cpu/memory.hpp
        source/platform/sdl/main.cpp
        source/core/memory.hpp
        source/core/memory.cpp
        source/core/cpu/instructions.cpp
        source/core/ppu/ppu.hpp
        source/core/gameboy.hpp
        source/core/ppu/ppu.cpp
        source/core/irq.hpp
        source/core/mbc/mbc.hpp
        source/core/mbc/no_mbc.hpp
        source/core/mbc/mbc3.hpp
        source/core/irq.cpp
        source/core/joypad.hpp
        source/core/joypad.cpp
        source/core/scheduler.hpp
        source/core/timer.hpp
        source/core/timer.cpp
        source/core/apu/channel/sequencer.hpp
        source/core/apu/channel/channel_noise.hpp
        source/core/apu/channel/channel_noise.cpp
        source/core/apu/channel/channel_quad.hpp
        source/core/apu/channel/channel_quad.cpp
        source/core/apu/channel/channel_wave.hpp
        source/core/apu/channel/channel_wave.cpp
        source/core/apu/apu.hpp source/core/apu/apu.cpp
        source/core/apu/callback.cpp source/core/mbc/backup-file.hpp)
target_include_directories(ReBoy PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(ReBoy ${SDL2_LIBRARY} stdc++fs)
