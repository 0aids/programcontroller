#include "cursors.hpp"
#include "server.hpp"
#include <iterator>
#include <wayland-server-core.h>

extern "C" {
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/region.h>
}
#include <iostream>

void CursorManager::cursorMotionResponder(wl_listener *listener, void *data) {
  CursorManager *self = wl_container_of(listener, self, m_cursorMotionListener);
  // std::cout << "\rNew cursorMotion detected" << std::endl;
  auto *mouseEvent = static_cast<struct wlr_pointer_motion_event *>(data);

  auto dx = mouseEvent->delta_x, dy = mouseEvent->delta_y;

  double sx, sy;
  auto *node = wlr_scene_node_at(
      &self->m_parentServer->m_sceneManager.m_scene->tree.node,
      self->m_cursor->x, self->m_cursor->y, &sx, &sy);

  wlr_pointer_constraint_v1 *constraint = nullptr;
  time_t t;
  time(&t);

  // This is required in order for games to react to camera dragging motions.
  wlr_relative_pointer_manager_v1_send_relative_motion(
      self->m_relativePointerManager,
      self->m_parentServer->m_inputManager.m_seat, t, mouseEvent->delta_x,
      mouseEvent->delta_y, mouseEvent->unaccel_dx, mouseEvent->unaccel_dy);

  // This part is to ensure the game has focus at all times. I have no clue how
  // it works
  wl_list_for_each(constraint, &self->m_pointerConstraints->constraints, link) {
  }
  if (constraint &&
      !wl_list_empty(&self->m_parentServer->m_topLevelsManager.m_topLevels_l)) {

    TopLevelView *main_view = wl_container_of(
        self->m_parentServer->m_topLevelsManager.m_topLevels_l.next, main_view,
        m_link);
    wlr_surface *target_surface = main_view->m_topLevel->base->surface;

    double sx_confined, sy_confined;
    if (wlr_region_confine(&constraint->region, sx, sy,
                           sx + mouseEvent->delta_x, sy + mouseEvent->delta_y,
                           &sx_confined, &sy_confined)) {
      std::cout << "Would've been out of region, confining..." << std::endl;
      dx = sx_confined - sx;
      dy = sx_confined - sy;
    }
  }

  wlr_cursor_move(self->m_cursor, &mouseEvent->pointer->base, dx, dy);

  wlr_seat_pointer_notify_motion(self->m_parentServer->m_inputManager.m_seat,
                                 mouseEvent->time_msec, sx, sy);
  // Show the actual cursor
  // wlr_cursor_set_xcursor(
  //     self->m_parentServer->m_inputManager.m_cursorManager.m_cursor,
  //     self->m_parentServer->m_inputManager.m_cursorManager.m_xcursorManager,
  //     "default");
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
}

void CursorManager::cursorButtonResponder(wl_listener *listener, void *data) {
  CursorManager *self = wl_container_of(listener, self, m_cursorButtonListener);
  std::cout << "New cursorButton detected" << std::endl;
  auto *mouseEvent = static_cast<struct wlr_pointer_button_event *>(data);

  wlr_seat_pointer_notify_button(self->m_parentServer->m_inputManager.m_seat,
                                 mouseEvent->time_msec, mouseEvent->button,
                                 mouseEvent->state);
}

void CursorManager::requestSetCursorResponder(wl_listener *listener,
                                              void *data) {
  CursorManager *self =
      wl_container_of(listener, self, m_requestSetCursorListener);
  auto *event = static_cast<wlr_seat_pointer_request_set_cursor_event *>(data);
  std::cout << "New cursor set request" << std::endl;

  // This is the client asking to set the cursor.
  // We need to honor this request.

  // Check that the seat client asking for the change is the one with pointer
  // focus.
  wlr_cursor_set_surface(self->m_cursor, event->surface, event->hotspot_x,
                         event->hotspot_y);
}
void CursorConstraint::destroyResponder(wl_listener *listener, void *data) {
  CursorConstraint *self = wl_container_of(listener, self, m_destroyListener);

  wl_list_remove(&self->m_destroyListener.link);

  delete self;
}

void CursorManager::newConstraintResponder(wl_listener *listener, void *data) {
  CursorManager *self =
      wl_container_of(listener, self, m_newConstraintListener);
  auto *constraint = static_cast<wlr_pointer_constraint_v1 *>(data);
  std::cout << "New constraint received of type: " << constraint->type
            << std::endl;

  CursorConstraint *cursorConstraint = new CursorConstraint();
  cursorConstraint->m_constraint = constraint;
  cursorConstraint->m_destroyListener.notify =
      CursorConstraint::destroyResponder;

  wlr_pointer_constraint_v1_send_activated(cursorConstraint->m_constraint);

  wl_signal_add(&cursorConstraint->m_constraint->events.destroy,
                &cursorConstraint->m_destroyListener);
}

void CursorManager::init(WaylandServer *parentServer) {
  m_parentServer = parentServer;
  m_cursor = wlr_cursor_create();

  wlr_cursor_attach_output_layout(m_cursor,
                                  parentServer->m_outputManager.m_outputLayout);

  m_xcursorManager = wlr_xcursor_manager_create(NULL, 24);

  m_cursorMotionListener.notify = cursorMotionResponder;
  // m_cursorMotionAbsoluteListener.notify = cursorMotionAbsoluteResponder;
  m_cursorButtonListener.notify = cursorButtonResponder;
  m_requestSetCursorListener.notify = requestSetCursorResponder;

  wl_signal_add(&m_cursor->events.motion, &m_cursorMotionListener);
  // wl_signal_add(&m_cursor->events.motion_absolute,
  //               &m_cursorMotionAbsoluteListener);
  wl_signal_add(&m_cursor->events.button, &m_cursorButtonListener);
  wl_signal_add(
      &m_parentServer->m_inputManager.m_seat->events.request_set_cursor,
      &m_requestSetCursorListener);

  m_pointerConstraints =
      wlr_pointer_constraints_v1_create(m_parentServer->m_display);

  m_relativePointerManager =
      wlr_relative_pointer_manager_v1_create(m_parentServer->m_display);

  m_newConstraintListener.notify = newConstraintResponder;

  wl_signal_add(&m_pointerConstraints->events.new_constraint,
                &m_newConstraintListener);
}
