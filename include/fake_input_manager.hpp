#pragma once

#include "queue.hpp"
#include <chrono>
#include <functional>
#include <thread>
#include <wayland-server-protocol.h>
extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/backend.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
}

class WLR_State;

using Keycode    = unsigned int;
using Buttoncode = unsigned int;

enum e_KeyState {
    KEY_PRESS   = WL_KEYBOARD_KEY_STATE_PRESSED,
    KEY_RELEASE = WL_KEYBOARD_KEY_STATE_RELEASED,
};

enum e_ButtonState {
    MOUSE_PRESS   = WL_POINTER_BUTTON_STATE_PRESSED,
    MOUSE_RELEASE = WL_POINTER_BUTTON_STATE_RELEASED,
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
// Absolute fucking monstrosity.
constexpr auto CLEARQUEUE = std::chrono::steady_clock::time_point{};

using InputInstructionFunc = std::function<void()>;

struct InputInstruction {
    InputInstructionFunc                  function;
    std::chrono::steady_clock::time_point processTime;
};

using InputDuration = std::chrono::nanoseconds;

/* Should we hook into the signals and activate them?
 * Hooking into the signals and activating them rather than directly notifying the seat
 * NOTE: Currently we are hooking into signals only for the mouse, as otherwise 
 * there would be a whole bunch of extra shit I would have to deal with.
 *
 *  Pros:
 *      Less boilerplate code needed.
 *  Cons:
 *      Less flexibility
 *
 *  I want the FakeInputManager to not only control the inputs, 
 *  but the entire state programatically, such as focusing windows,
 *  opening windows, etc. Maybe this can be under the global state 
 *  controllers instead.
 *
 *  Plan:
 *    - The managers will have public facing API that allows simplified 
 *      access and modification to the global state.
 *    - The managers can be controlled programatically to turn on and off 
 *      desired features.
 *    - The managers can modify global state, but cannot construct or destruct it.
 * */

class FakeInputManager {

  private:
    WLR_State*                  d_state;
    std::thread                 m_inputProcessingThread;
    SPSCQueue<InputInstruction> m_instructionQueue;
    bool                        m_keepInputProcessingThreadAlive;

    // If we send an instruction with timestamp 0, we clear the queue.
    void watchAndProcessQueue();

  public:
    FakeInputManager(WLR_State* state);
    ~FakeInputManager();
    void sendKey(Keycode keycode, e_KeyState state,
                 InputDuration delay);

    void holdKeyForDuration(Keycode keycode, InputDuration duration,
                            InputDuration delay);

    void sendMouseButton(Buttoncode button, e_ButtonState state,
                         InputDuration delay);

    void holdMouseButtonForDuration(Buttoncode    button,
                                    InputDuration duration,
                                    InputDuration delay);

    void moveMouseDelta(double dx, double dy, InputDuration delay);
    void moveMouseAbsolute(double x, double y, InputDuration delay);

    void moveMouseDeltaInterpolate(double dx, double dy,
                                   size_t        numSamples,
                                   InputDuration duration,
                                   InputDuration delay);

    // Interpolates from the current position to x and y.
    // Only sends absolute motion events.
    void moveMouseAbsoluteInterpolate(double x, double y,
                                      size_t        numSamples,
                                      InputDuration duration,
                                      InputDuration delay);

    void sendTestInstruction();
    void sendCursorTest();

    void killInputProcessingThread();

    void sendClearQueue();
};
