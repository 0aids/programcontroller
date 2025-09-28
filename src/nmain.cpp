#include "fake_input_manager.hpp"
#include "server.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

extern "C" {
#include <wlr/util/log.h>
}

void randomshit(InputManager *manager) {
  FakeInputManager inputManager(manager);

  std::string in = "";
  std::cout << "Randomshit thread started!" << std::endl;

  std::uniform_real_distribution<double> unif(-1000, 1000);
  std::default_random_engine re;

  while (in != "q") {
    std::cin >> in;
    using namespace std::chrono;
    std::cout << "Sending request" << std::endl;
    auto dx = unif(re);
    auto dy = unif(re);
    inputManager.moveMouseDeltaInterpolate(dx, dy, 100, 1s, 1s);
  }

  inputManager.killInputProcessingThread();
}

int main() {
  wlr_log_init(WLR_DEBUG, NULL);
  WaylandServer server;
  server.start();
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", "org.vinegarhq.Sober", (void *)NULL);
  }

  std::thread newThread(randomshit, &server.m_inputManager);

  server.loop();

  return 0;
}
