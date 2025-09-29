#pragma once
#include "input.hpp"
#include "output.hpp"
#include "scenes.hpp"
#include "toplevel.hpp"

extern "C" {
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_subcompositor.h>
}

class WaylandServer {
  public:
    OutputManager    m_outputManager;

    SceneManager     m_sceneManager;

    InputManager     m_inputManager;

    TopLevelsManager m_topLevelsManager;

    wl_display*      m_display;
    wlr_backend*     m_backend;
    wlr_renderer*    m_renderer;
    wlr_allocator*   m_allocator;
    wl_event_loop*   m_eventLoop;
    const char*      waylandSocket;

  public:
    WaylandServer(/* No options for now*/);
    ~WaylandServer(/* No options for now*/);

    // Start the backend, renderer, allocator and displays.
    void start();

    // Start the main event loop
    void loop();
};
