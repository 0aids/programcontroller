#include "fake_input_manager.hpp"
#include "server.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

extern "C" {
#include <wlr/util/log.h>
}

void randomshit(wlr_seat *seat) {
  FakeInputManager inputManager(seat);

  std::string in = "";
  std::cout << "Randomshit thread started!" << std::endl;

  while (in != "q") {
    std::cin >> in;
    std::cout << "Sending request" << std::endl;
    inputManager.sendTestInstruction();
  }

  inputManager.killInputProcessingThread();
}

int main() {
  wlr_log_init(WLR_DEBUG, NULL);
  WaylandServer server;
  server.start();
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", "foot", (void *)NULL);
  }

  std::thread newThread(randomshit, server.m_inputManager.m_seat);

  server.loop();

  return 0;
}
