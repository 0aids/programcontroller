#include "fake_input_manager.hpp"
#include "log.hpp"
#include "server.hpp"
#include <cmath>
#include <fcntl.h>
#include "managers.hpp"
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/util/log.h>
}
// I should really add a shell-like thing for debugging.

void randomshit(WLR_State* state) {
    using namespace std::chrono;
    FakeInputManager inputManager(state);

    std::string      in = "";
    Log(Debug, "Starting randomshit thread");

    std::uniform_real_distribution<double> unif(-1000, 1000);
    std::default_random_engine             re;

    std::cout << "Waiting for something" << std::endl;
    std::cin >> in;

    // inputManager.sendKey(KEY_W, KEY_PRESS, 0s);
    while (true) {
        inputManager.moveMouseDeltaInterpolate(1000, 0, 100, 1s, 1s);
        inputManager.moveMouseAbsolute(0.5, 0.5, 500ms);
        inputManager.moveMouseAbsolute(0.5, 0.5, 1000ms);
        std::this_thread::sleep_for(1s);
    }

    // while (in != "q") {
    //     std::cin >> in;
    //     using namespace std::chrono;
    //     Log(Debug, "Sending request");
    //     if (in == "1") {
    //         inputManager.moveMouseDeltaInterpolate(500, 500, 100, 1s,
    //                                                1s);
    //     } else if (in == "2") {
    //         inputManager.moveMouseAbsoluteInterpolate(0.1, 0.1, 100,
    //                                                   1s, 1s);
    //     } else if (in == "3") {
    //         inputManager.moveMouseDelta(10, 10, 1s);
    //     } else if (in == "4") {
    //         inputManager.moveMouseAbsolute(0.9, 0.9, 1s);
    //     } else if (in == "5") {
    //         inputManager.holdKeyForDuration(KEY_W, 1s, 1s);
    //     } else if (in == "6") {
    //         inputManager.holdMouseButtonForDuration(BTN_RIGHT, 1s,
    //                                                 1s);
    //     } else if (in == "9")
    //         inputManager.sendClearQueue();
    // }

    inputManager.killInputProcessingThread();
}

int main() {
    wlr_log_init(WLR_INFO, NULL);
    WLR_State state;
    Managers  manager;
    state.init();
    manager.init(&state);
    state.m_managers = &manager;
    state.start();
    if (fork() == 0) {
        // int fd_null = open("/dev/null", O_WRONLY);
        // if (dup2(fd_null, STDOUT_FILENO) == -1) {
        //     perror("dup2 STDOUT");
        //     close(fd_null);
        //     _exit(1);
        // }
        // if (dup2(fd_null, STDERR_FILENO) == -1) {
        //     perror("dup2 STDERR");
        //     close(fd_null);
        //     _exit(1);
        // }

        execl("/bin/sh", "/bin/sh", "-c", "org.vinegarhq.Sober",
              (void*)NULL);
    }

    std::thread newThread(randomshit, &state);

    state.loop();

    return 0;
}
