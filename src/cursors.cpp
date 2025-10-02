#include "cursors.hpp"
#include "log.hpp"
#include "server.hpp"
#include <wayland-server-core.h>
#include <mutex>
#include "toplevel.hpp"

extern "C" {
#include <wayland-util.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/region.h>
}
#include <iostream>
void CursorManager::cursorMotion(
    wlr_pointer_motion_event* mouseEvent) {
    std::lock_guard<std::mutex> lock(counterMutex);

    auto   dx = mouseEvent->delta_x, dy = mouseEvent->delta_y;

    double sx, sy;
    auto*  node =
        wlr_scene_node_at(&d_state->Scenes.m_scene->tree.node,
                          d_state->Cursors.m_cursor->x,
                          d_state->Cursors.m_cursor->y, &sx, &sy);

    wlr_pointer_constraint_v1* constraint = nullptr;
    time_t                     t;
    time(&t);

    wlr_seat_pointer_notify_enter(
        d_state->Core.m_seat,
        d_state->UserState.m_cursorFocusedSurface, sx, sy);

    // This is required in order for games to react to camera dragging motions.
    wlr_relative_pointer_manager_v1_send_relative_motion(
        d_state->Cursors.m_relativePointerManager,
        d_state->Core.m_seat, t, mouseEvent->delta_x,
        mouseEvent->delta_y, mouseEvent->unaccel_dx,
        mouseEvent->unaccel_dy);

    // This part is to ensure the game has focus at all times. I have no clue how
    // it works, i just stole it from dwl.
    wl_list_for_each(
        constraint,
        &d_state->Cursors.m_pointerConstraints->constraints, link) {}
    if (constraint &&
        !wl_list_empty(&d_state->TopLevels.m_topLevels_l)) {

        TopLevelView* main_view = wl_container_of(
            d_state->TopLevels.m_topLevels_l.next, main_view, m_link);
        wlr_surface* target_surface =
            main_view->m_topLevel->base->surface;

        double sx_confined, sy_confined;
        // This part isn't even reached?? I never see the print statement
        if (wlr_region_confine(&constraint->region, sx, sy,
                               sx + mouseEvent->delta_x,
                               sy + mouseEvent->delta_y, &sx_confined,
                               &sy_confined)) {
            Log(Debug, "Would've been out of region, confining...");
            dx = sx_confined - sx;
            dy = sx_confined - sy;
        }
    }

    wlr_cursor_move(d_state->Cursors.m_cursor,
                    d_state->Cursors.m_cursorDevice, dx, dy);

    wlr_seat_pointer_notify_motion(d_state->Core.m_seat,
                                   mouseEvent->time_msec, sx, sy);
}

void CursorManager::cursorMotionResponder(wl_listener* listener,
                                          void*        data) {
    // 2. Create a lock_guard object
    // When lock_guard is created, it calls counter_mutex.lock().
    CursorManager* self =
        wl_container_of(listener, self, m_cursorMotionListener);
    Log(Debug, "New cursorMotion detected");
    auto* mouseEvent =
        static_cast<struct wlr_pointer_motion_event*>(data);

    self->cursorMotion(mouseEvent);
}

void CursorManager::cursorMotionAbsoluteResponder(
    wl_listener* listener, void* data) {
    CursorManager* self = wl_container_of(
        listener, self, m_cursorMotionAbsoluteListener);
    Log(Debug, "New cursorMotionAbsolute detected");

    auto* mouseEvent =
        static_cast<struct wlr_pointer_motion_absolute_event*>(data);

    // Make sure the cursor doesn't get offsetted too far.
    wlr_cursor_warp_absolute(self->d_state->Cursors.m_cursor,
                             self->d_state->Cursors.m_cursorDevice,
                             mouseEvent->x, mouseEvent->y);
}

void CursorManager::cursorButtonResponder(wl_listener* listener,
                                          void*        data) {
    CursorManager* self =
        wl_container_of(listener, self, m_cursorButtonListener);
    Log(Debug, "New cursorButton detected");
    auto* mouseEvent =
        static_cast<struct wlr_pointer_button_event*>(data);

    wlr_seat_pointer_notify_button(
        self->d_state->Core.m_seat, mouseEvent->time_msec,
        mouseEvent->button, mouseEvent->state);
}

void CursorManager::requestSetCursorResponder(wl_listener* listener,
                                              void*        data) {
    CursorManager* self =
        wl_container_of(listener, self, m_requestSetCursorListener);
    auto* event =
        static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
    Log(Debug, "New cursor set request");

    // This is the client asking to set the cursor.
    // We need to honor this request.

    // Check that the seat client asking for the change is the one with pointer
    // focus.
    wlr_cursor_set_surface(self->d_state->Cursors.m_cursor,
                           event->surface, event->hotspot_x,
                           event->hotspot_y);
}
void CursorConstraint::destroyResponder(wl_listener* listener,
                                        void*        data) {
    CursorConstraint* self =
        wl_container_of(listener, self, m_destroyListener);

    wl_list_remove(&self->m_destroyListener.link);

    delete self;
}

void CursorManager::newConstraintResponder(wl_listener* listener,
                                           void*        data) {
    CursorManager* self =
        wl_container_of(listener, self, m_newConstraintListener);
    auto* constraint = static_cast<wlr_pointer_constraint_v1*>(data);
    Log(Debug,
        "New constraint received of type: " << constraint->type);

    CursorConstraint* cursorConstraint = new CursorConstraint();
    cursorConstraint->m_constraint     = constraint;
    cursorConstraint->m_destroyListener.notify =
        CursorConstraint::destroyResponder;

    wlr_pointer_constraint_v1_send_activated(
        cursorConstraint->m_constraint);

    wl_signal_add(&cursorConstraint->m_constraint->events.destroy,
                  &cursorConstraint->m_destroyListener);
}

void CursorManager::init(WLR_State* state) {
    Log(Debug, "Initiaizing the cursor manager");
    d_state                       = state;
    m_cursorMotionListener.notify = cursorMotionResponder;
    m_cursorMotionAbsoluteListener.notify =
        cursorMotionAbsoluteResponder;
    m_cursorButtonListener.notify     = cursorButtonResponder;
    m_requestSetCursorListener.notify = requestSetCursorResponder;

    wl_signal_add(&d_state->Cursors.m_cursor->events.motion,
                  &m_cursorMotionListener);
    wl_signal_add(&d_state->Cursors.m_cursor->events.motion_absolute,
                  &m_cursorMotionAbsoluteListener);
    wl_signal_add(&d_state->Cursors.m_cursor->events.button,
                  &m_cursorButtonListener);
    wl_signal_add(&d_state->Core.m_seat->events.request_set_cursor,
                  &m_requestSetCursorListener);

    m_newConstraintListener.notify = newConstraintResponder;

    wl_signal_add(
        &d_state->Cursors.m_pointerConstraints->events.new_constraint,
        &m_newConstraintListener);
}

void CursorManager::newCursor(wlr_input_device* device) {
    Log(Debug, "Initializing new cursor");
    d_state->Cursors.m_cursorDevice = device;
    wlr_cursor_attach_input_device(d_state->Cursors.m_cursor, device);
}
