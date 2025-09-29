#include <atomic>
#include <chrono>
#include <cstdlib> // For rand() and srand()
#include <iostream>
#include <semaphore>
#include <sys/types.h>
#include <thread>
#include <vector>

// The head manages the beginning of the queue
// The tail is the end of the queue.
// If m_tail == m_head, then empty
// If m_tail + 1 == m_head, then full.
// The tail should contain the next empty.
template <typename T>
class SPSCQueue {
  private:
    const size_t   m_size;
    std::vector<T> m_arr;
    alignas(64) std::atomic<size_t> m_tail;
    alignas(64) std::atomic<size_t> m_head;

  public:
    std::counting_semaphore<> m_semaphore{0};
    SPSCQueue(size_t size) : m_size(size), m_arr(std::vector<T>(size)), m_tail(0), m_head(0) {}

    // False for failed, True for success.
    bool enqueue(T input) {
        const auto head = m_head.load(std::memory_order_acquire);

        const auto tail = m_tail.load(std::memory_order_relaxed);
        // Even if the race condition occurs, then we will just ignore it and say
        // that it failed
        const auto nextTail = (tail + 1) % m_size;
        if (nextTail == head) {
            std::cerr << "Queue is full" << std::endl;
            return false;
        }

        m_arr[tail] = input;

        m_tail.store(nextTail, std::memory_order_release);
        m_semaphore.release();

        return true;
    }

    bool isEmpty() {
        return (m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire));
    }

    bool acquiredDequeue(T& resultReference) {
        // We are the only one that can modify this.
        const auto head = m_head.load(std::memory_order_relaxed);

        // Ensure that we get the correct tail.
        const auto tail = m_tail.load(std::memory_order_acquire);
        if (head == tail) {
            return false;
        }
        resultReference = m_arr[head];
        m_head.store((head + 1) % m_size, std::memory_order_release);
        return true;
    }

    bool dequeue(T& resultReference) {
        // Read the head and tail.
        // We need to make sure we get the correct head and tail.

        // We are the only one that can modify this.
        const auto head = m_head.load(std::memory_order_relaxed);

        // Ensure that we get the correct tail.
        const auto tail = m_tail.load(std::memory_order_acquire);

        if (head == tail) {
            return false;
        }

        // Should be near instant.
        m_semaphore.acquire();

        resultReference = m_arr[head];
        m_head.store((head + 1) % m_size, std::memory_order_release);

        return true;
    }

    T semaphoreBlockingDequeue() {
        m_semaphore.acquire();

        // We are the only one that can modify this.
        const auto head = m_head.load(std::memory_order_relaxed);

        // Ensure that we get the correct tail.
        const auto tail   = m_tail.load(std::memory_order_acquire);
        auto       result = m_arr[head];
        m_head.store((head + 1) % m_size, std::memory_order_release);
        return result;
    }

    T pollingBlockingDequeue(std::chrono::nanoseconds ns) {
        T result;
        while (true) {
            if (dequeue(result)) {
                return result;
            }
            std::this_thread::sleep_for(ns);
        }
    }
};
