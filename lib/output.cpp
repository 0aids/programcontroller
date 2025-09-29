#include "output.hpp"
#include "log.hpp"
#include "server.hpp"
#include <iostream>
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
}
void WaylandOutput::newFrameResponder(wl_listener *listener, void *data) {
  // Log(Debug, "Frame request received!");
  WaylandOutput *self = wl_container_of(listener, self, m_newFrameListener);
  // This is the rendering loop.
  struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
      self->m_parentServer->m_sceneManager.m_scene, self->m_output);

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
void WaylandOutput::newStateResponder(wl_listener *listener, void *data) {
  WaylandOutput *self = wl_container_of(listener, self, m_newStateListener);
  // Used for changing the state of an ouput, depending on the output's
  // request.
  Log(Debug, "Received a State Request!");
  //
  auto *event =
      static_cast<const struct wlr_output_event_request_state *>(data);
  // The host compositor is asking us to change our mode . We just need to
  // accept it by committing the suggested state.
  wlr_output_commit_state(self->m_output, event->state);
}
void WaylandOutput::DestroyRequestResponder(wl_listener *listener, void *data) {
  // Nothing for now
}

WaylandOutput::WaylandOutput(OutputManager *outputManager, wlr_output *output) {
  m_output = output;
  // This line was effectively missing. It connects this output
  // back to the main server object.
  m_parentServer = outputManager->m_parentServer;

  m_newFrameListener.notify = newFrameResponder;
  // m_server = parentServer; // This line seems redundant if you use
  // m_parentServer consistently
  wl_signal_add(&m_output->events.frame, &m_newFrameListener);

  m_newStateListener.notify = newStateResponder;
  wl_signal_add(&m_output->events.request_state, &m_newStateListener);

  m_destroyRequestListener.notify = DestroyRequestResponder;
  wl_signal_add(&m_output->events.destroy, &m_destroyRequestListener);
  // Adds a the new output from left to right because i'm fucking lazy.
  struct wlr_output_layout_output *layoutOutput =
      wlr_output_layout_add_auto(outputManager->m_outputLayout, m_output);

  struct wlr_scene_output *sceneOutput =
      wlr_scene_output_create(m_parentServer->m_sceneManager.m_scene, m_output);

  wlr_scene_output_layout_add_output(
      m_parentServer->m_sceneManager.m_sceneLayout, layoutOutput, sceneOutput);
}

// static
void OutputManager::newOutputHandler(wl_listener *listener, void *data) {
  Log(Debug, "New output detected!");
  OutputManager *self = wl_container_of(listener, self, m_newOutputListener);
  auto *newOutput = static_cast<wlr_output *>(data);

  wlr_output_init_render(newOutput, self->m_parentServer->m_allocator,
                         self->m_parentServer->m_renderer);
  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  // Set the output mode (resolution and refreshrate and shit).
  // TODO: Set it to a custom resolution.
  struct wlr_output_mode *mode = wlr_output_preferred_mode(newOutput);
  if (mode != NULL) {
    wlr_output_state_set_mode(&state, mode);
  }

  wlr_output_commit_state(newOutput, &state);
  wlr_output_state_finish(&state);

  // This will manage itself i hope.
  WaylandOutput *out = new WaylandOutput(self, newOutput);
}

void OutputManager::init(WaylandServer *parentServer, wl_display *display) {
  m_parentServer = parentServer;
  m_outputLayout = wlr_output_layout_create(display);
  wl_list_init(&m_outputs_l);
  m_newOutputListener.notify = newOutputHandler;
  wl_signal_add(&m_parentServer->m_backend->events.new_output,
                &m_newOutputListener);
}
