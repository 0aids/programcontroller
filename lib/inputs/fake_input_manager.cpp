#include "fake_input_manager.hpp"
#include <chrono>
#include <queue>
#include <semaphore>
#include <thread>

extern "C" {
#include <linux/input-event-codes.h>
}

FakeInputManager::FakeInputManager(wlr_seat *seat)
    : m_seat(seat), m_keyboard(wlr_seat_get_keyboard(seat)),
      m_instructionQueue(16384) {
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

void FakeInputManager::watchAndProcessQueue() {
  std::priority_queue<InputInstruction, std::vector<InputInstruction>,
                      CompareInstruction>
      pq;

  // Process the queue
  // We'll go for millisecond accuracy.
  using namespace std::chrono;
  duration waitDuration = milliseconds::max();
  // std::cout << "Watching and processing queue!" << std::endl;
  while (m_keepInputProcessingThreadAlive) {

    // Determine how long we should wait for.
    auto now = steady_clock::now();
    if (pq.empty()) {
      waitDuration = 10000ms;
    } else if (pq.top().processTime > now) {
      waitDuration = duration_cast<milliseconds>(pq.top().processTime - now);
    } else {
      waitDuration = 0ms;
    }
    // std::cout << "Current wait duration: " << waitDuration << std::endl;
    if (m_instructionQueue.m_semaphore.try_acquire_for(waitDuration)) {
      // std::cout << "Received new instruction" << std::endl;
      InputInstruction newReq;
      if (m_instructionQueue.acquiredDequeue(newReq)) {
        pq.push(newReq);
      }
    }
    now = steady_clock::now();
    while (!pq.empty() && pq.top().processTime <= now) {
      InputInstruction instruction = pq.top();
      pq.pop();
      instruction.function();
    }
  }
}

void FakeInputManager::sendTestInstruction() {
  using namespace std::chrono;
  auto now = steady_clock::now();
  auto func = [now]() -> void {
    std::cout << "Time since epoch in ms: "
              << duration_cast<milliseconds>(now.time_since_epoch())
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
