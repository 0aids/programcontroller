#include "log.hpp"
#include "server.hpp"

void WLR_State::Core::init() {
    Log(Debug, "Initializing WLR_State Core.");
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

    m_compositor    = wlr_compositor_create(m_display, 5, m_renderer);
    m_subCompositor = wlr_subcompositor_create(m_display);
    m_dataDeviceManager = wlr_data_device_manager_create(m_display);

    m_seat = wlr_seat_create(m_display, "seat99");
}

void WLR_State::Outputs::init(WLR_State& state) {
    Log(Debug, "Initializing WLR_State Output.");
    wl_list_init(&m_outputs_l);
    m_outputLayout = wlr_output_layout_create(state.Core.m_display);
}

void WLR_State::Scenes::init(WLR_State& state) {
    Log(Debug, "Initializing WLR_State Scenes.");
    m_scene       = wlr_scene_create();
    m_sceneLayout = wlr_scene_attach_output_layout(
        m_scene, state.Outputs.m_outputLayout);
}

void WLR_State::TopLevels::init(WLR_State& state) {
    Log(Debug, "Initializing WLR_State Toplevels.");
    wl_list_init(&m_topLevels_l);
    m_xdgShell = wlr_xdg_shell_create(state.Core.m_display, 3);
}

void WLR_State::Cursors::init(WLR_State& state) {
    Log(Debug, "Initializing WLR_State Cursors.");
    m_cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(m_cursor,
                                    state.Outputs.m_outputLayout);

    m_xcursorManager = wlr_xcursor_manager_create(NULL, 24);

    m_pointerConstraints =
        wlr_pointer_constraints_v1_create(state.Core.m_display);

    m_relativePointerManager =
        wlr_relative_pointer_manager_v1_create(state.Core.m_display);
}

void WLR_State::Keyboards::init(WLR_State& state) {
    Log(Debug, "Initializing WLR_State keyboards.");
    wl_list_init(&m_keyboards_l);
}

void WLR_State::init() {
    Log(Debug, "Initializing WLR_State.");
    Core.init();

    Outputs.init(*this);

    Scenes.init(*this);

    TopLevels.init(*this);

    Cursors.init(*this);

    Keyboards.init(*this);
}

WLR_State::~WLR_State() {
    Log(Debug, "Destroying WLR_State.");
    wl_display_destroy_clients(Core.m_display);
    wlr_scene_node_destroy(&Scenes.m_scene->tree.node);
    wlr_xcursor_manager_destroy(Cursors.m_xcursorManager);
    wlr_cursor_destroy(Cursors.m_cursor);
    wlr_allocator_destroy(Core.m_allocator);
    wlr_renderer_destroy(Core.m_renderer);
    wlr_backend_destroy(Core.m_backend);
    wl_display_destroy(Core.m_display);
}

void WLR_State::start() {
    Log(Debug, "Starting WLR_State.");
    m_waylandSocket = wl_display_add_socket_auto(Core.m_display);

    if (!m_waylandSocket) {
        wlr_backend_destroy(Core.m_backend);
        Log(Error, "Failed to initialise the wayland socket.");
        exit(EXIT_FAILURE);
    }

    /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
    if (!wlr_backend_start(Core.m_backend)) {
        wlr_backend_destroy(Core.m_backend);
        wl_display_destroy(Core.m_display);
        Log(Error, "Failed to start the backend.");
        exit(EXIT_FAILURE);
    }

    /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
    setenv("WAYLAND_DISPLAY", m_waylandSocket, true);
    Log(Message,
        "Running Wayland compositor on WAYLAND_DISPLAY="
            << m_waylandSocket);
}

void WLR_State::loop() {
    Log(Debug, "Running the main loop");
    while (1) {
        wl_event_loop_dispatch(Core.m_eventLoop, -1);
        wl_display_flush_clients(Core.m_display);
    }
}
