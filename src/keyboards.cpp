#include "keyboards.hpp"
#include "log.hpp"
#include "server.hpp"

extern "C" {
#include <wayland-util.h>
}

#include <iostream>

void KeyboardManager::keyboardInputResponder(wl_listener* listener,
                                             void*        data) {
    KeyboardManager* self =
        wl_container_of(listener, self, m_keyboardInputListener);
    Log(Debug, "New keyboard input detected");

    auto* event = static_cast<struct wlr_keyboard_key_event*>(data);
    auto* keyboard =
        wlr_seat_get_keyboard(self->d_state->Core.m_seat);
    wlr_seat_keyboard_notify_modifiers(self->d_state->Core.m_seat,
                                       &keyboard->modifiers);
    wlr_seat_keyboard_notify_key(self->d_state->Core.m_seat,
                                 event->time_msec, event->keycode,
                                 event->state);
}

void KeyboardManager::init(WLR_State* state) {
    //
    Log(Debug, "Initializing the Keyboard Manager");
    d_state = state;
}

void KeyboardManager::newKeyboard(wlr_input_device* device) {

    wlr_keyboard* keyboard;
    Log(Debug, "New keyboard detected");
    keyboard         = wlr_keyboard_from_input_device(device);
    auto* xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    auto* xkbKeymap  = xkb_keymap_new_from_names(
        xkbContext, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

    // Setup the keyboard to also send keysyms alongside input.
    wlr_keyboard_set_keymap(keyboard, xkbKeymap);
    xkb_keymap_unref(xkbKeymap);
    xkb_context_unref(xkbContext);
    wlr_keyboard_set_repeat_info(keyboard, 25, 600);

    wlr_seat_set_keyboard(d_state->Core.m_seat, keyboard);
    m_keyboardInputListener.notify = keyboardInputResponder;
    wl_signal_add(&keyboard->events.key, &m_keyboardInputListener);
}
