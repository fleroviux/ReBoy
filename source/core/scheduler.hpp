/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>
#include <functional>

class Scheduler {
public:
  struct Event {
    std::function<void(int)> callback;
  private:
    friend class Scheduler;
    int handle;
    std::uint64_t timestamp;
  };

  Scheduler() {
    for (int i = 0; i < kMaxEvents; i++) {
      heap[i] = new Event();
      heap[i]->handle = i;
    }
    Reset();
  }

  ~Scheduler() {
    for (int i = 0; i < kMaxEvents; i++) {
      delete heap[i];
    }
  }

  void Reset() {
    heap_size = 0;
    timestamp_now = 0;
  }

  auto GetTimestampNow() const -> std::uint64_t {
    return timestamp_now;
  }

  auto GetTimestampTarget() const -> std::uint64_t {
    return heap[0]->timestamp;
  }

  auto GetRemainingCycleCount() const -> int {
    return int(GetTimestampTarget() - GetTimestampNow());
  }

  void AddCycles(int cycles) {
    timestamp_now += cycles;
  }

  auto Add(std::uint64_t delay, std::function<void(int)> callback) -> Event* {
    int n = heap_size++;
    int p = Parent(n);

    auto event = heap[n];
    event->timestamp = GetTimestampNow() + delay;
    event->callback = callback;

    while (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
      Swap(n, p);
      n = p;
      p = Parent(n);
    }

    return event;
  }

  void Cancel(Event* event) {
    Remove(event->handle);
  }

  void Step() {
    auto now = GetTimestampNow();
    while (heap_size > 0 && heap[0]->timestamp <= now) {
      auto event = heap[0];
      Remove(0);
      event->callback(int(now - event->timestamp));
    }
  }

private:
  static constexpr int kMaxEvents = 64;

  constexpr int Parent(int n) { return (n - 1) / 2; }
  constexpr int LeftChild(int n) { return n * 2 + 1; }
  constexpr int RightChild(int n) { return n * 2 + 2; }

  void Remove(int n) {
    Swap(n, --heap_size);

    int p = Parent(n);
    if (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
      do {
        Swap(n, p);
        n = p;
        p = Parent(n);
      } while (n != 0 && heap[p]->timestamp > heap[n]->timestamp);
    } else {
      Heapify(n);
    }
  }

  void Swap(int i, int j) {
    auto tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    heap[i]->handle = i;
    heap[j]->handle = j;
  }

  void Heapify(int n) {
    int l = LeftChild(n);
    int r = RightChild(n);

    if (l < heap_size && heap[l]->timestamp < heap[n]->timestamp) {
      Swap(l, n);
      Heapify(l);
    }

    if (r < heap_size && heap[r]->timestamp < heap[n]->timestamp) {
      Swap(r, n);
      Heapify(r);
    }
  }

  Event* heap[kMaxEvents];
  int heap_size;
  std::uint64_t timestamp_now;
};
