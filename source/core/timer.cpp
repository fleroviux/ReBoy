/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "timer.hpp"

void Timer::Reset() {
  div = 255;
  tima = 0;
  tma = 0;
  tac = {};
  timer_event = nullptr;
  StepDIV(0);
}

void Timer::StepDIV(int cycles_late) {
  div++;
  scheduler->Add(256 - cycles_late, [this](int cycles_late){StepDIV(cycles_late);});
}

void Timer::StepTimer(int cycles_late) {
  if (tima == 255) {
    tima = tma;
    irq->Raise(IRQ::TIMER);
  } else {
    tima++;
  }

  // TODO: do we really need to check if the timer has been disabled?
  if (tac.enabled)
    ScheduleTimer(cycles_late);
}

void Timer::ScheduleTimer(int cycles_late) {
  static constexpr int kTimerDuty[4] {
    1024, 16, 64, 256 };
  auto cycles = kTimerDuty[static_cast<int>(tac.clock_select)] - cycles_late;
  timer_event = scheduler->Add(cycles, [this](int cycles_late){StepTimer(cycles_late);});
}

auto Timer::ReadMMIO(std::uint8_t reg) -> std::uint8_t {
  switch (reg) {
    case REG_DIV:
      return div;
    case REG_TIMA:
      return tima;
    case REG_TMA:
      return tma;
    case REG_TAC:
      return static_cast<std::uint8_t>(tac.clock_select) |
             (tac.enabled ? 4 : 0);
  }
  return 0;
}

void Timer::WriteMMIO(std::uint8_t reg, std::uint8_t value) {
  switch (reg) {
    case REG_DIV:
      div = 0;
      break;
    case REG_TIMA:
      // TODO: does the actually write the TIMA value or do something else?
      tima = value;
      break;
    case REG_TMA:
      tma = value;
      break;
    case REG_TAC:
      auto enabled_old = tac.enabled;
      auto clock_select_old = tac.clock_select;
      tac.clock_select = static_cast<TAC::Clock>(value & 3);
      tac.enabled = value & 4;
      if (tac.clock_select != clock_select_old && enabled_old && tac.enabled) {
        scheduler->Cancel(timer_event);
        ScheduleTimer(0);
      }
      // TODO: handle clock frequency change.
      if (!enabled_old && tac.enabled) {
        tima = tma;
        ScheduleTimer(0);
      } else if (enabled_old && !tac.enabled) {
        scheduler->Cancel(timer_event);
      }
      break;
  }
}
