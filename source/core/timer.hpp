/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include "scheduler.hpp"
#include "irq.hpp"

class Timer {
public:
  Timer(Scheduler* scheduler, IRQ* irq) : scheduler(scheduler), irq(irq) { Reset(); }

  void Reset();
  auto ReadMMIO(std::uint8_t reg) -> std::uint8_t;
  void WriteMMIO(std::uint8_t reg, std::uint8_t value);

private:
  enum Registers {
    REG_DIV = 0x04,
    REG_TIMA = 0x05,
    REG_TMA = 0x06,
    REG_TAC = 0x07
  };

  Scheduler* scheduler;
  IRQ* irq;
  std::uint8_t div;
  std::uint8_t tima;
  std::uint8_t tma;
  struct TAC {
    bool enabled = false;
    enum class Clock {
      _4096 = 0,
      _262144 = 1,
      _65536 = 2,
      _16384 = 3
    } clock_select = Clock::_4096;
  } tac;
  Scheduler::Event* timer_event;

  void StepDIV(int cycles_late);
  void StepTimer(int cycles_late);
  void ScheduleTimer(int cycles_late);
};
