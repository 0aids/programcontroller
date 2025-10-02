#include "toplevel.hpp"
#include "log.hpp"
#include "server.hpp"
#include <iostream>

extern "C" {
#include <wayland-util.h>
#include <wlr/util/log.h>
}

void TopLevelView::commitResponder(wl_listener* listener,
                                   void*        data) {
    TopLevelView* self =
        wl_container_of(listener, self, m_commitListener);
    if (!self->m_topLevel->base->initial_commit &&
        (self->fullscreenAttempts++ >
         self->maximumFullscreenAttemptAmount)) {
        return;
    }
    Log(Debug, "Received commit request");
    // 1. Reliably get the primary (and only) output from the layout.
    // We don't ask the client where it is; we find the display ourselves.
    struct wlr_output* output = wlr_output_layout_get_center_output(
        self->d_state->Outputs.m_outputLayout);
    if (!output) {
        Log(Error,
            "Could not find an output to place the toplevel on.");
        return;
    }

    // 2. Get the exact dimensions and position of that output in the layout.
    // This is the authoritative geometry we will enforce.
    struct wlr_box output_box;
    wlr_output_layout_get_box(self->d_state->Outputs.m_outputLayout,
                              output, &output_box);

    // 3. Configure the client to be fullscreen and match the output size.
    wlr_xdg_toplevel_set_fullscreen(self->m_topLevel, true);
    wlr_xdg_toplevel_set_size(self->m_topLevel, output_box.width,
                              output_box.height);
    // 4. Position our scene node to align perfectly with the output.
    wlr_scene_node_set_position(&self->m_sceneTree->node,
                                output_box.x, output_box.y);

    // 5. Set focus.
    wlr_xdg_toplevel_set_activated(self->m_topLevel, true);
    double                 sx, sy;
    struct wlr_scene_node* node = wlr_scene_node_at(
        &self->d_state->Scenes.m_scene->tree.node,
        self->d_state->Cursors.m_cursor->x,
        self->d_state->Cursors.m_cursor->y, &sx, &sy);

    self->d_state->UserState.m_cursorFocusedSurface =
        self->m_topLevel->base->surface;
    self->d_state->UserState.m_keyboardFocusedSurface =
        self->m_topLevel->base->surface;

    wlr_seat_pointer_notify_enter(
        self->d_state->Core.m_seat,
        self->d_state->UserState.m_cursorFocusedSurface, sx, sy);

    auto* keyboard =
        wlr_seat_get_keyboard(self->d_state->Core.m_seat);
    if (keyboard) {
        wlr_seat_keyboard_notify_enter(
            self->d_state->Core.m_seat,
            self->m_topLevel->base->surface, keyboard->keycodes,
            keyboard->num_keycodes, &keyboard->modifiers);
    }

    self->m_sceneTree->node.enabled = true;
}

TopLevelView::TopLevelView(WLR_State*        state,
                           wlr_xdg_toplevel* topLevel) {
    m_topLevel = topLevel;
    d_state    = state;
    wl_list_insert(&d_state->TopLevels.m_topLevels_l, &m_link);
    // Create a scene graph node for this toplevel.
    // The scene graph will manage rendering the surface.
    m_sceneTree = wlr_scene_xdg_surface_create(
        &d_state->Scenes.m_scene->tree, topLevel->base);

    // Initially, the window is not mapped/visible.
    m_sceneTree->node.enabled = false;
    m_commitListener.notify   = commitResponder;
    wl_signal_add(&m_topLevel->base->surface->events.commit,
                  &m_commitListener);
}

void TopLevelsManager::newTopLevelResponder(wl_listener* listener,
                                            void*        data) {
    Log(Debug, "New toplevel detected");
    TopLevelsManager* self =
        wl_container_of(listener, self, m_topLevelListener);
    auto* topLevel = static_cast<struct wlr_xdg_toplevel*>(data);
    auto* newView  = new TopLevelView(self->d_state, topLevel);
}
void TopLevelsManager::newTopLevelPopupResponder(
    wl_listener* listener, void* data) {
    // Do nothing
}

void TopLevelsManager::init(WLR_State* state) {
    d_state = state;

    m_topLevelListener.notify = newTopLevelResponder;
    // m_topLevelPopupListener.notify = newTopLevelResponder;

    wl_signal_add(&d_state->TopLevels.m_xdgShell->events.new_toplevel,
                  &m_topLevelListener);
}
