// WARNING: Some games cannot take more than certain amount of inputs at once.
// Doing more than the allowed will cause the game to crash (cough cough
// roblox).
#include "fake_input_manager.hpp"
#include "input.hpp"
#include "log.hpp"
#include <chrono>
#include <ctime>
#include <queue>
#include <semaphore>
#include <thread>

extern "C" {
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
}

InputInstructionFunc
FakeInputManager::generateRelativeMouseInstructionFunc(double dx, double dy) {
  time_t currTime;
  time(&currTime);
  auto func = [this, currTime, dx, dy]() -> void {
    auto *server = this->m_inputManager->m_parentServer;

    Log(Debug, "dx: " << dx << "\tdy: " << dy);
    wlr_relative_pointer_manager_v1_send_relative_motion(
        m_inputManager->m_cursorManager.m_relativePointerManager,
        m_inputManager->m_seat, currTime, dx, dy, dx, dy);
  };
  return func;
}

InputInstructionFunc
FakeInputManager::generateKeyboardInstructionFunc(Keycode keycode,
                                                  e_KeyState state) {
  auto func = [this, keycode, state]() {
    time_t currTime;
    time(&currTime);
    auto *seat = this->m_inputManager->m_seat;
    wlr_seat_keyboard_notify_key(seat, currTime, keycode, state);
  };
  return func;
}

void FakeInputManager::holdKeyForDuration(Keycode keycode,
                                          InputDurationNanoseconds duration,
                                          InputDurationNanoseconds delay) {
  // We'll send the higher-level seat notification.
  auto now = std::chrono::steady_clock::now();
  InputInstruction pressInstruction;
  pressInstruction.function = generateKeyboardInstructionFunc(keycode, PRESS);
  pressInstruction.processTime = now + delay;

  InputInstruction releaseInstruction;
  releaseInstruction.function =
      generateKeyboardInstructionFunc(keycode, RELEASE);
  releaseInstruction.processTime = now + delay + duration;

  m_instructionQueue.enqueue(pressInstruction);
  m_instructionQueue.enqueue(releaseInstruction);
}

FakeInputManager::FakeInputManager(InputManager *inputManager)
    : m_inputManager(inputManager),
      m_keyboard(wlr_seat_get_keyboard(m_inputManager->m_seat)),
      m_cursor(m_inputManager->m_cursorManager.m_cursor),
      m_instructionQueue(65536) {
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
  bool operator()(const InputInstruction &a, const InputInstruction &b) {
    return a.processTime > b.processTime;
  }
};

void FakeInputManager::moveMouseDeltaInterpolate(
    double totalDx, double totalDy, size_t numSamples,
    InputDurationNanoseconds duration, InputDurationNanoseconds delay) {
  // Create the list of instructions
  std::vector<InputInstruction> instructions;
  instructions.reserve(numSamples);
  auto interval = duration / numSamples;
  double newDx = (totalDx / numSamples);
  double newDy = (totalDy / numSamples);
  for (size_t i = 0; i < numSamples; i++) {
    instructions.push_back(InputInstruction{
        .function = generateRelativeMouseInstructionFunc(newDx, newDy),
        .processTime = std::chrono::steady_clock::now() + interval * i + delay,
    });
  }

  for (const auto &inst : instructions) {
    m_instructionQueue.enqueue(inst);
  }
}

// TODO: I could add an enforcement or an option for 1ms delay between
// instructions.
void FakeInputManager::watchAndProcessQueue() {
  // Why the fuck can't you reserve a pq of a specific size? actually pieces of
  // shit.
  std::priority_queue<InputInstruction, std::vector<InputInstruction>,
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
      waitDuration = duration_cast<nanoseconds>(pq.top().processTime - now);
    } else {
      waitDuration = 0ns;
    }
    if (m_instructionQueue.m_semaphore.try_acquire_for(waitDuration)) {
      InputInstruction newReq;
      if (m_instructionQueue.acquiredDequeue(newReq)) {

        if (newReq.processTime == CLEARQUEUE_DURATION) {
          // Dump everything.
          Log(Debug, "Clearing the queue");
          pq = std::priority_queue<InputInstruction,
                                   std::vector<InputInstruction>,
                                   CompareInstruction>();
        }
        pq.push(newReq);
      }
    }
    if (!pq.empty()) {
      Log(Debug, "Queue size: " << pq.size());
      now = steady_clock::now();
    }

    int totalProcessed = 0;
    while (!pq.empty() && pq.top().processTime <= now) {
      InputInstruction instruction = pq.top();
      instruction.function();
      // The instructions are processed with <0.2ms delay.
      Log(Debug, "Late by: " << steady_clock::now() - instruction.processTime);
      pq.pop();
    }
  }
}

void FakeInputManager::sendCursorTest() {
  using namespace std::chrono;
  const size_t numIter = 200;
  Log(Debug, "Sending cursor test of size: " << numIter);
  std::vector<InputInstruction> instructions;
  auto now = std::chrono::steady_clock::now();
  instructions.reserve(numIter);
  const double range = 0.4;
  const double d = 10;
  for (size_t i = 0; i < numIter; i++) {
    time_t currTime;
    time(&currTime);
    auto func = [this, d, currTime]() -> void {
      wlr_relative_pointer_manager_v1_send_relative_motion(
          m_inputManager->m_cursorManager.m_relativePointerManager,
          m_inputManager->m_seat, currTime, 1, 0, 1, 0);
    };
    auto designatedTime = now + 5ms * i + 1s;
    instructions.push_back(
        InputInstruction{.function = func, .processTime = designatedTime});
  }

  int i = 0;
  for (const auto &inst : instructions) {
    Log(Debug, "Sending instruction: " << i++);
    m_instructionQueue.enqueue(inst);
  }
}

void FakeInputManager::sendTestInstruction() {
  using namespace std::chrono;
  auto now = steady_clock::now();
  auto func = [now]() -> void {
    std::cout << "Time since epoch in ms: "
              << duration_cast<nanoseconds>(now.time_since_epoch())
              << std::endl;
  };
  InputInstruction instruction = {
      .function = func,
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
      CLEARQUEUE_DURATION,
  });
}
