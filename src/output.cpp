#include "output.hpp"
#include "log.hpp"
#include "server.hpp"
#include <iostream>
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
}
void WaylandOutput::newFrameResponder(wl_listener* listener,
                                      void*        data) {
    // Log(Debug, "Frame request received!");
    WaylandOutput* self =
        wl_container_of(listener, self, m_newFrameListener);
    // This is the rendering loop.
    struct wlr_scene_output* scene_output =
        wlr_scene_get_scene_output(self->d_state->Scenes.m_scene,
                                   self->m_output);

    // Render the scene if needed.
    if (!wlr_scene_output_commit(scene_output, NULL)) {
        return;
    }

    // Send a frame done event to the client.
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    wlr_scene_output_send_frame_done(scene_output, &now);
    //
}
void WaylandOutput::newStateResponder(wl_listener* listener,
                                      void*        data) {
    WaylandOutput* self =
        wl_container_of(listener, self, m_newStateListener);
    // Used for changing the state of an ouput, depending on the output's
    // request.
    Log(Debug, "Received a State Request!");
    //
    auto* event =
        static_cast<const struct wlr_output_event_request_state*>(
            data);
    // The host compositor is asking us to change our mode . We just need to
    // accept it by committing the suggested state.
    wlr_output_commit_state(self->m_output, event->state);
}
void WaylandOutput::DestroyRequestResponder(wl_listener* listener,
                                            void*        data) {
    // Nothing for now
}

WaylandOutput::WaylandOutput(WLR_State* state, wlr_output* output) {
    d_state                   = state;
    m_output                  = output;
    m_newFrameListener.notify = newFrameResponder;
    wl_signal_add(&m_output->events.frame, &m_newFrameListener);

    m_newStateListener.notify = newStateResponder;
    wl_signal_add(&m_output->events.request_state,
                  &m_newStateListener);

    m_destroyRequestListener.notify = DestroyRequestResponder;
    wl_signal_add(&m_output->events.destroy,
                  &m_destroyRequestListener);
}

// static
void OutputManager::newOutputHandler(wl_listener* listener,
                                     void*        data) {
    // This is just a factory that creates outputs, so we let the actual waylandOutput object only handle signals.
    Log(Debug, "New output detected!");
    OutputManager* self =
        wl_container_of(listener, self, m_newOutputListener);
    auto* newOutput = static_cast<wlr_output*>(data);

    wlr_output_init_render(newOutput, self->d_state->Core.m_allocator,
                           self->d_state->Core.m_renderer);
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    // Set the output mode (resolution and refreshrate and shit).
    // TODO: Set it to a custom resolution.
    struct wlr_output_mode* mode =
        wlr_output_preferred_mode(newOutput);
    if (mode != NULL) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(newOutput, &state);
    wlr_output_state_finish(&state);

    // Adds a the new output from left to right because i'm fucking lazy.
    struct wlr_output_layout_output* layoutOutput =
        wlr_output_layout_add_auto(
            self->d_state->Outputs.m_outputLayout, newOutput);

    struct wlr_scene_output* sceneOutput = wlr_scene_output_create(
        self->d_state->Scenes.m_scene, newOutput);

    wlr_scene_output_layout_add_output(
        self->d_state->Scenes.m_sceneLayout, layoutOutput,
        sceneOutput);

    // This will manage itself i hope. this is what they essentially did for tinywl.
    // TODO: Check for fixes
    WaylandOutput* out = new WaylandOutput(self->d_state, newOutput);
}

void OutputManager::init(WLR_State* state) {
    Log(Debug, "Initializing the output manager");
    d_state = state;

    m_newOutputListener.notify = newOutputHandler;
    wl_signal_add(&state->Core.m_backend->events.new_output,
                  &m_newOutputListener);
}
