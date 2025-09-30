#include "fake_input_manager.hpp"
#include "log.hpp"
#include "server.hpp"
#include <cmath>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/util/log.h>
}

void randomshit(InputManager* manager) {
    FakeInputManager inputManager(manager);

    std::string      in = "";
    Log(Debug, "Starting randomshit thread");

    std::uniform_real_distribution<double> unif(-1000, 1000);
    std::default_random_engine             re;

    while (in != "q") {
        std::cin >> in;
        using namespace std::chrono;
        Log(Debug, "Sending request");
        auto dx = unif(re);
        auto dy = unif(re);
        if (in == "a")
            inputManager.moveMouseDeltaInterpolate(dx, dy, 100, 1s,
                                                   1s);
        else if (in == "b")
            inputManager.holdKeyForDuration(KEY_W, 1s, 1s);
        else if (in == "c")
            inputManager.sendClearQueue();
    }

    inputManager.killInputProcessingThread();
}

int main() {
    WaylandServer server;
    server.start();
    if (fork() == 0) {
        int fd_null = open("/dev/null", O_WRONLY);
        if (dup2(fd_null, STDOUT_FILENO) == -1) {
            perror("dup2 STDOUT");
            close(fd_null);
            _exit(1);
        }
        if (dup2(fd_null, STDERR_FILENO) == -1) {
            perror("dup2 STDERR");
            close(fd_null);
            _exit(1);
        }

        execl("/bin/sh", "/bin/sh", "-c", "org.vinegarhq.Sober",
              (void*)NULL);
    }

    std::thread newThread(randomshit, &server.m_inputManager);

    server.loop();

    return 0;
}
