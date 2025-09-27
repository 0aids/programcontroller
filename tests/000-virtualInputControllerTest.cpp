#include "fake_input_manager.hpp"
#include <chrono>
#include <thread>

extern "C" {
#include <wlr/types/wlr_seat.h>
}

int main() { wlr_seat fake_seat; };
