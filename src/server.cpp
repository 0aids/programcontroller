// TODO: Switch to headless.
extern "C" {
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>
}
#include "log.hpp"
#include "server.hpp"

WaylandServer::WaylandServer() {
    m_display = wl_display_create();

    if (!m_display) {
        Log(Error, "Failed to create display!");
        exit(EXIT_FAILURE);
    }

    m_eventLoop = wl_display_get_event_loop(m_display);

    if (!m_eventLoop) {
        Log(Error, "Failed to initialize eventLoop!");
        exit(EXIT_FAILURE);
    }

    m_backend = wlr_backend_autocreate(m_eventLoop, NULL);
    if (!m_backend) {
        Log(Error, "Failed to initialize backend!");
        exit(EXIT_FAILURE);
    }

    m_renderer = wlr_renderer_autocreate(m_backend);
    if (!m_renderer) {
        Log(Error, "Failed to initialize renderer!");
        exit(EXIT_FAILURE);
    }

    if (!wlr_renderer_init_wl_display(m_renderer, m_display)) {
        Log(Error, "Failed to initialize display!");
        exit(EXIT_FAILURE);
    }

    m_allocator = wlr_allocator_autocreate(m_backend, m_renderer);

    if (!m_allocator) {
        Log(Error, "Failed to initialize allocator!");
        exit(EXIT_FAILURE);
    }

    wlr_compositor_create(m_display, 5, m_renderer);
    wlr_subcompositor_create(m_display);
    wlr_data_device_manager_create(m_display);

    // NOTE: Initialize the output and its manager
    m_outputManager.init(this, m_display);

    // NOTE: Scene initialization
    m_sceneManager.init(this);

    // NOTE: Toplevel and shell initialization
    m_topLevelsManager.init(this);

    // NOTE: Input initialization
    m_inputManager.init(this);
}

WaylandServer::~WaylandServer() {
    // Do nothing for now
}

void WaylandServer::start() {
    waylandSocket = wl_display_add_socket_auto(m_display);

    if (!waylandSocket) {
        wlr_backend_destroy(m_backend);
        Log(Error, "Failed to initialise the wayland socket.");
        exit(EXIT_FAILURE);
    }

    /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
    if (!wlr_backend_start(m_backend)) {
        wlr_backend_destroy(m_backend);
        wl_display_destroy(m_display);
        Log(Error, "Failed to start the backend.");
        exit(EXIT_FAILURE);
    }

    /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
    setenv("WAYLAND_DISPLAY", waylandSocket, true);
    Log(Message,
        "Running Wayland compositor on WAYLAND_DISPLAY="
            << waylandSocket);
}

void WaylandServer::loop() {
    Log(Debug, "Running the main loop");
    while (1) {
        wl_event_loop_dispatch(m_eventLoop, -1);
        wl_display_flush_clients(m_display);
    }
}
