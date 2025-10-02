// WARNING: Some games cannot take more than certain amount of inputs at once.
// Doing more than the allowed will cause the game to crash (cough cough
// roblox).
#include "fake_input_manager.hpp"
#include "managers.hpp"
#include "input.hpp"
#include "log.hpp"
#include "server.hpp"
#include <chrono>
#include <ctime>
#include <queue>
#include <semaphore>
#include <thread>
#include <wayland-server-protocol.h>

extern "C" {
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
}

using namespace std::chrono_literals;
using namespace std::chrono;

static inline time_t getCurrentTime() {
    time_t a;
    time(&a);
    return a;
}

InputInstructionFunc static inline generateMouseButtonFunc(
    Buttoncode button, e_ButtonState buttonState, WLR_State* state) {
    wlr_pointer_button_event event = {
        .pointer   = nullptr,
        .time_msec = 0,
        .button    = button,
        .state     = (wl_pointer_button_state)buttonState,
    };
    wl_signal* mouseButtonSignal =
        &state->Cursors.m_cursor->events.button;

    auto func = [event, mouseButtonSignal]() -> void {
        wl_signal_emit(mouseButtonSignal, (void*)(&event));
    };
    return func;
}

InputInstructionFunc static inline generateRelativeMouseInstructionFunc(
    double dx, double dy, WLR_State* state) {
    // Construct the mouse event var
    wlr_pointer_motion_event* event = new wlr_pointer_motion_event{
        .pointer    = nullptr,
        .time_msec  = 0,
        .delta_x    = dx,
        .delta_y    = dy,
        .unaccel_dx = dx,
        .unaccel_dy = dy,
    };
    // Cannot use wl_signal because it will cause race conditions with the other threads.
    // These race conditions cause the pc to go fucking haywire because the lambda is malformed.
    auto func = [event, state]() -> void {
        state->m_managers->m_cursorMgr.cursorMotion(event);
        delete event;
    };
    return func;
}

InputInstructionFunc static inline generateAbsoluteMouseInstructionFunc(
    double x, double y, WLR_State* state) {
    auto                              currentTime = getCurrentTime();
    wlr_pointer_motion_absolute_event event       = {
              .pointer   = nullptr,
              .time_msec = 0,
              .x         = x,
              .y         = y,
    };
    wl_signal* mouseAbsoluteMotionSignal =
        &state->Cursors.m_cursor->events.motion_absolute;
    auto func = [event, mouseAbsoluteMotionSignal]() -> void {
        // NOTE: Static casting does not work.
        wl_signal_emit(mouseAbsoluteMotionSignal, (void*)(&event));
    };

    return func;
}

InputInstructionFunc static inline generateKeyboardInstructionFunc(
    Keycode keycode, e_KeyState keyState, WLR_State* state) {
    auto* seat = state->Core.m_seat;
    auto  func = [seat, keycode, keyState, state]() {
        wlr_seat_keyboard_notify_key(seat, getCurrentTime(), keycode,
                                      keyState);
    };
    return func;
}

void FakeInputManager::holdKeyForDuration(Keycode       keycode,
                                          InputDuration duration,
                                          InputDuration delay) {
    // We'll send the higher-level seat notification.
    auto             now = std::chrono::steady_clock::now();
    InputInstruction pressInstruction;
    pressInstruction.function =
        generateKeyboardInstructionFunc(keycode, KEY_PRESS, d_state);
    pressInstruction.processTime = now + delay;

    InputInstruction releaseInstruction;
    releaseInstruction.function = generateKeyboardInstructionFunc(
        keycode, KEY_RELEASE, d_state);
    releaseInstruction.processTime = now + delay + duration;

    m_instructionQueue.enqueue(pressInstruction);
    m_instructionQueue.enqueue(releaseInstruction);
}

void FakeInputManager::sendKey(Keycode keycode, e_KeyState state,
                               InputDuration delay) {
    m_instructionQueue.enqueue({
        .function =
            generateKeyboardInstructionFunc(keycode, state, d_state),
        .processTime = steady_clock::now() + delay,
    });
}

void FakeInputManager::sendMouseButton(Buttoncode    button,
                                       e_ButtonState buttonState,
                                       InputDuration delay) {
    auto func = generateMouseButtonFunc(button, buttonState, d_state);

    auto time = steady_clock::now() + delay;

    m_instructionQueue.enqueue(InputInstruction{
        .function    = func,
        .processTime = time,
    });
}

void FakeInputManager::holdMouseButtonForDuration(
    Buttoncode button, InputDuration duration, InputDuration delay) {
    // We'll send the higher-level seat notification.
    auto             now = std::chrono::steady_clock::now();
    InputInstruction pressInstruction;
    pressInstruction.function =
        generateMouseButtonFunc(button, MOUSE_PRESS, d_state);
    pressInstruction.processTime = now + delay;

    InputInstruction releaseInstruction;
    releaseInstruction.function =
        generateMouseButtonFunc(button, MOUSE_RELEASE, d_state);
    releaseInstruction.processTime = now + delay + duration;

    m_instructionQueue.enqueue(pressInstruction);
    m_instructionQueue.enqueue(releaseInstruction);
}

FakeInputManager::FakeInputManager(WLR_State* state) :
    // You better not fucking exhaust 65536 of them. This should be more than plenty.
    d_state(state), m_instructionQueue(65536) {
    m_inputProcessingThread =
        std::thread(&FakeInputManager::watchAndProcessQueue, this);
    m_keepInputProcessingThreadAlive = true;
}
FakeInputManager::~FakeInputManager() {
    m_keepInputProcessingThreadAlive = false;

    m_inputProcessingThread.join();
    // We do not own anything else so we just ignore them.
}

struct CompareInstruction {
    bool operator()(const InputInstruction& a,
                    const InputInstruction& b) {
        return a.processTime > b.processTime;
    }
};

void FakeInputManager::watchAndProcessQueue() {
    // Why the fuck can't you reserve a pq of a specific size? actually pieces of
    // shit.
    std::priority_queue<InputInstruction,
                        std::vector<InputInstruction>,
                        CompareInstruction>
        pq;

    // Process the queue
    using namespace std::chrono;
    duration waitDuration = nanoseconds::max();
    Log(Debug, "Watching and processing queue!");
    while (m_keepInputProcessingThreadAlive) {

        // Determine how long we should wait for.
        auto now = steady_clock::now();
        if (pq.empty()) {
            waitDuration = 10s; // Setting to max makes it angry.
        } else if (pq.top().processTime > now) {
            waitDuration = duration_cast<nanoseconds>(
                pq.top().processTime - now);
        } else {
            waitDuration = 0ns;
        }
        if (m_instructionQueue.m_semaphore.try_acquire_for(
                waitDuration)) {
            InputInstruction newReq;
            if (m_instructionQueue.acquiredDequeue(newReq)) {

                if (newReq.processTime == CLEARQUEUE) {
                    // Dump everything.
                    Log(Debug, "Clearing the queue");
                    pq = std::priority_queue<
                        InputInstruction,
                        std::vector<InputInstruction>,
                        CompareInstruction>();
                }
                pq.push(newReq);
            }
        }
        if (!pq.empty()) {
            // Log(Debug, "Queue size: " << pq.size());
            now = steady_clock::now();
        }

        int totalProcessed = 0;
        while (!pq.empty() && pq.top().processTime <= now) {
            InputInstruction instruction = pq.top();
            instruction.function();
            // The instructions are processed with <0.2ms delay.
            // Log(Debug,
            //     "Late by: " << steady_clock::now() -
            //             instruction.processTime);
            pq.pop();
        }
    }
}

void FakeInputManager::sendCursorTest() {
    const size_t numIter = 200;
    Log(Debug, "Sending cursor test of size: " << numIter);
    std::vector<InputInstruction> instructions;
    auto now = std::chrono::steady_clock::now();
    instructions.reserve(numIter);
    const double range = 0.4;
    const double d     = 10;
    for (size_t i = 0; i < numIter; i++) {
        time_t currTime;
        time(&currTime);
        auto func = [this, d, currTime]() -> void {
            wlr_relative_pointer_manager_v1_send_relative_motion(
                d_state->Cursors.m_relativePointerManager,
                d_state->Core.m_seat, currTime, 1, 0, 1, 0);
        };
        auto designatedTime = now + 5ms * i + 1s;
        instructions.push_back(InputInstruction{
            .function = func, .processTime = designatedTime});
    }

    int i = 0;
    for (const auto& inst : instructions) {
        Log(Debug, "Sending instruction: " << i++);
        m_instructionQueue.enqueue(inst);
    }
}

void FakeInputManager::moveMouseDelta(double dx, double dy,
                                      InputDuration delay) {
    auto func = generateRelativeMouseInstructionFunc(dx, dy, d_state);
    auto time = steady_clock::now() + delay;
    m_instructionQueue.enqueue(InputInstruction{
        .function    = func,
        .processTime = time,
    });
}

void FakeInputManager::moveMouseDeltaInterpolate(
    double totalDx, double totalDy, size_t numSamples,
    InputDuration duration, InputDuration delay) {
    // Create the list of instructions
    std::vector<InputInstruction> instructions;
    instructions.reserve(numSamples);
    auto   interval = duration / numSamples;
    double newDx    = (totalDx / numSamples);
    double newDy    = (totalDy / numSamples);
    for (size_t i = 0; i < numSamples; i++) {
        instructions.push_back(InputInstruction{
            .function = generateRelativeMouseInstructionFunc(
                newDx, newDy, d_state),
            .processTime = std::chrono::steady_clock::now() +
                interval * i + delay,
        });
    }

    for (const auto& inst : instructions) {
        m_instructionQueue.enqueue(inst);
    }
}

void FakeInputManager::moveMouseAbsolute(double x, double y,
                                         InputDuration delay) {
    auto func = generateAbsoluteMouseInstructionFunc(x, y, d_state);
    auto time = steady_clock::now() + delay;
    m_instructionQueue.enqueue(InputInstruction{
        .function    = func,
        .processTime = time,
    });
}

void FakeInputManager::moveMouseAbsoluteInterpolate(
    double x, double y, size_t numSamples, InputDuration duration,
    InputDuration delay) {
    // Create the list of instructions
    std::vector<InputInstruction> instructions;
    instructions.reserve(numSamples);
    auto   interval = duration / numSamples;

    double curX = d_state->Cursors.m_cursor->x;
    double curY = d_state->Cursors.m_cursor->y;

    double dx = (x - curX) / numSamples;
    double dy = (y - curY) / numSamples;

    for (size_t i = 1; i <= numSamples; i++) {
        instructions.push_back(InputInstruction{
            .function = generateAbsoluteMouseInstructionFunc(
                curX + (dx * i), curY + (dy * i), d_state),
            .processTime = std::chrono::steady_clock::now() +
                interval * i + delay,
        });
    }

    for (const auto& inst : instructions) {
        m_instructionQueue.enqueue(inst);
    }
}

void FakeInputManager::sendTestInstruction() {
    using namespace std::chrono;
    auto now  = steady_clock::now();
    auto func = [now]() -> void {
        Log(Debug,
            "Time since epoch in ms: " << duration_cast<nanoseconds>(
                now.time_since_epoch()));
    };
    InputInstruction instruction = {
        .function    = func,
        .processTime = now,
    };
    m_instructionQueue.enqueue(instruction);
}

void FakeInputManager::killInputProcessingThread() {
    m_keepInputProcessingThreadAlive = false;
}

void FakeInputManager::sendClearQueue() {
    m_instructionQueue.enqueue(InputInstruction{
        []() {},
        CLEARQUEUE,
    });
}
