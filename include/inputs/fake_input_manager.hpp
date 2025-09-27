#pragma once

#include "queue.hpp"
#include <chrono>
#include <functional>
#include <thread>
extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/backend.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/types/wlr_seat.h>
}

using Keycode = unsigned int;

enum e_KeyState {
  PRESS = WL_KEYBOARD_KEY_STATE_PRESSED,
  RELEASE = WL_KEYBOARD_KEY_STATE_RELEASED,
};

// We need a struct to pass all the data of a move instruction.
// For a button instruction we can either:
//  1. key down
//  2. key up
//  3. mouse button down
//  4. mouse button up
//
// For a move instruction we need:
//  1. Mouse move dxdy (delta)
//  2. Mouse move xy (absolute)
//
// Alternatively, I can just send the instruction pointers with the arguments?
// I can construct lambdas with the data with arguments already incorporated.
// And dump that into a struct with the appropriate time stamp.

using InputInstructionFunc = std::function<void()>;
struct InputInstruction {
  InputInstructionFunc function;
  std::chrono::steady_clock::time_point processTime;
};

using InputDurationMilliseconds =
    std::chrono::duration<std::chrono::milliseconds>;

class FakeInputManager {

private:
  wlr_seat *m_seat;
  wlr_keyboard *m_keyboard;
  std::thread m_inputProcessingThread;
  SPSCQueue<InputInstruction> m_instructionQueue;
  bool m_keepInputProcessingThreadAlive;
  void watchAndProcessQueue();

public:
  FakeInputManager(wlr_seat *seat);
  ~FakeInputManager();
  void sendKey(Keycode keycode, e_KeyState state);
  // We want to be able to move the cursor. How should this be done?
  // Provide helper functions for moving the cursor in different manners

  void holdKey(Keycode keycode, InputDurationMilliseconds duration);

  void moveMouseDelta(uint64_t dx, uint64_t dy);
  void moveMouseAbsolute(uint64_t x, uint64_t y);

  void moveMouseDeltaInterpolate(uint64_t dx, uint64_t dy, size_t numSamples,
                                 InputDurationMilliseconds duration);

  void moveMouseAbsoluteInterpolate(uint64_t x, uint64_t y, size_t numSamples,
                                    InputDurationMilliseconds duration);

  void sendTestInstruction();
  void killInputProcessingThread();
};
