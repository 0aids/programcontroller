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
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
}

using Keycode = unsigned int;

enum e_KeyState {
    PRESS   = WL_KEYBOARD_KEY_STATE_PRESSED,
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
class InputManager;

// Use for time_point if you want to clear the queue instantly.
constexpr auto CLEARQUEUE_DURATION =
    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::steady_clock::duration>{
        std::chrono::steady_clock::duration{0}};

using InputInstructionFunc = std::function<void()>;

struct InputInstruction {
    InputInstructionFunc                  function;
    std::chrono::steady_clock::time_point processTime;
};

using InputDurationNanoseconds = std::chrono::nanoseconds;

/* Externally required:
 *      CursorManager . wlr_relative_pointer_manager_v1
 *      CursorManager . wlr_cursor
 *      InputManager -> wlr_seat
 *
 *      MAYBE
 *      KeyboardManager (but the seat is all that's needed to send keys).
 *      
 * */

class FakeInputManager {

  private:
    InputManager*               m_inputManager;
    wlr_keyboard*               m_keyboard;
    wlr_cursor*                 m_cursor;
    std::thread                 m_inputProcessingThread;
    SPSCQueue<InputInstruction> m_instructionQueue;
    bool                        m_keepInputProcessingThreadAlive;

    // If we send an instruction with timestamp 0, we clear the queue.
    void watchAndProcessQueue();

    InputInstructionFunc
    generateRelativeMouseInstructionFunc(double dx, double dy);

    InputInstructionFunc
    generateKeyboardInstructionFunc(Keycode    keycode,
                                    e_KeyState state);

  public:
    FakeInputManager(InputManager* inputManager);
    ~FakeInputManager();
    void sendKey(Keycode keycode, e_KeyState state);

    void holdKeyForDuration(Keycode                  keycode,
                            InputDurationNanoseconds duration,
                            InputDurationNanoseconds delay);

    void moveMouseDelta(double dx, double dy);
    void moveMouseAbsolute(double x, double y);

    void moveMouseDeltaInterpolate(double dx, double dy,
                                   size_t numSamples,
                                   InputDurationNanoseconds duration,
                                   InputDurationNanoseconds delay);

    void
         moveMouseAbsoluteInterpolate(double x, double y,
                                      size_t                   numSamples,
                                      InputDurationNanoseconds duration,
                                      InputDurationNanoseconds delay);

    void sendTestInstruction();
    void sendCursorTest();

    void killInputProcessingThread();

    void sendClearQueue();
};
