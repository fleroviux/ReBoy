# ReBoy
A basic Nintendo Game Boy emulator. Not particularily accurate or good, just a fun excercise.

## Timeline

The first functioning version of ReBoy was put together in roughly seven days.
Some of the code, namely the scheduler and most of the audio code, was borrowed from my GBA emulator [NanoboyAdvance](https://github.com/fleroviux/NanoboyAdvance).

- Day 1: Project setup, we can run and render the BOOTROM!
  - setup project base: CPU core foundation, memory, load BOOTROM into memory
  - implemented CPU instructions until the BOOTROM worked
  - implemented basic PPU state machine and background renderer

- Day 2: Tetris!
  - CPU instruction bugfixes and added implemented more opcodes
  - Add support for interrupts such as the V-blank interrupt
  - Stubbed Joypad input to prevent Tetris from bootlooping
  
- Day 3: Pokémon Blue, albeit buggy, shows something on the screen!
  - setup system for memory bank controllers and implemented MBC3
  - implemented many more CPU opcodes and fixed some bugs

- Day 4: Pokémon Blue and Gold go in game!
  - fixed many CPU opcode bugs, passing most of blargg's CPU instruction tests (all except DAA and 02-interrupts)
  - implemented H-blank, V-blank and LY<>LYC-coincidence interrupts
  - moved PPU events onto a heap-based event scheduler
  - implemented timers
  - setup some audio infrastructure
  
- Day 5: Sound goes beep boop beep boop
  - debugged a video/audio desynchronization issue in the Pokémon Gold/Silver intro movie (not solved at this point)
  - finished up sound emulation
  - implemented PPU window rendering
  

- Day 6: PPU rendering! Also saving!
  - optimized PPU background and window renderers
  - implemented PPU sprite sorting and rendering 
  - implemented saving
  
- Day 7: Optimization and cleanup.
  - optimized audio resampling, using a cheaper filtering technique which prove good enough.
  - cleanup and making it somewhat usable...
