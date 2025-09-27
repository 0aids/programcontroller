#include "cursors.hpp"
#include "server.hpp"

extern "C" {
#include <wayland-util.h>
#include <wlr/types/wlr_xcursor_manager.h>
}
#include <iostream>

void CursorManager::cursorMotionResponder(wl_listener *listener, void *data) {
  CursorManager *self = wl_container_of(listener, self, m_cursorMotionListener);
  // std::cout << "\rNew cursorMotion detected" << std::endl;
  auto *mouseEvent = static_cast<struct wlr_pointer_motion_event *>(data);

  wlr_cursor_move(self->m_cursor, &mouseEvent->pointer->base,
                  mouseEvent->delta_x, mouseEvent->delta_y);
  double sx, sy;
  wlr_surface *surface = NULL;
  auto *node = wlr_scene_node_at(
      &self->m_parentServer->m_sceneManager.m_scene->tree.node,
      self->m_cursor->x, self->m_cursor->y, &sx, &sy);
  wlr_seat_pointer_notify_motion(self->m_parentServer->m_inputManager.m_seat,
                                 mouseEvent->time_msec, sx, sy);
}

void CursorManager::cursorMotionAbsoluteResponder(wl_listener *listener,
                                                  void *data) {
  CursorManager *self =
      wl_container_of(listener, self, m_cursorMotionAbsoluteListener);
  // std::cout << "New cursorMotionAbsolute detected" << std::endl;

  auto *mouseEvent =
      static_cast<struct wlr_pointer_motion_absolute_event *>(data);

  // Make sure the cursor doesn't get offsetted too far.
  wlr_cursor_warp_absolute(self->m_cursor, &mouseEvent->pointer->base,
                           mouseEvent->x, mouseEvent->y);

  // Show the actual cursor
  wlr_cursor_set_xcursor(
      self->m_parentServer->m_inputManager.m_cursorManager.m_cursor,
      self->m_parentServer->m_inputManager.m_cursorManager.m_xcursorManager,
      "default");
}

void CursorManager::cursorButtonResponder(wl_listener *listener, void *data) {
  CursorManager *self = wl_container_of(listener, self, m_cursorButtonListener);
  std::cout << "New cursorButton detected" << std::endl;
  auto *mouseEvent = static_cast<struct wlr_pointer_button_event *>(data);

  wlr_seat_pointer_notify_button(self->m_parentServer->m_inputManager.m_seat,
                                 mouseEvent->time_msec, mouseEvent->button,
                                 mouseEvent->state);
}

void CursorManager::init(WaylandServer *parentServer) {
  m_parentServer = parentServer;
  m_cursor = wlr_cursor_create();

  wlr_cursor_attach_output_layout(m_cursor,
                                  parentServer->m_outputManager.m_outputLayout);

  m_xcursorManager = wlr_xcursor_manager_create(NULL, 24);

  m_cursorMotionListener.notify = cursorMotionResponder;
  m_cursorMotionAbsoluteListener.notify = cursorMotionAbsoluteResponder;
  m_cursorButtonListener.notify = cursorButtonResponder;

  wl_signal_add(&m_cursor->events.motion, &m_cursorMotionListener);
  wl_signal_add(&m_cursor->events.motion_absolute,
                &m_cursorMotionAbsoluteListener);
  wl_signal_add(&m_cursor->events.button, &m_cursorButtonListener);
}
