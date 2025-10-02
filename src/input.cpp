#include "input.hpp"
#include "log.hpp"
#include "server.hpp"
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
}
#include <iostream>

void InputManager::newInputDeviceResponder(wl_listener* listener,
                                           void*        data) {
    InputManager* self =
        wl_container_of(listener, self, m_newInputDeviceListener);

    Log(Debug, "New input detected");
    auto* device = static_cast<struct wlr_input_device*>(data);
    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD:
            Log(Debug, "New Keyboard detected");
            self->m_newKeyboardResponder(device);
            break;

        case WLR_INPUT_DEVICE_POINTER:
            Log(Debug, "New pointer detected");
            // NOTE: This should be done by the cursor manager
            // wlr_cursor_attach_input_device(
            //     self->m_cursorManager.m_cursor, device);
            self->m_newCursorResponder(device);
            break;

        default:
            Log(Warning, "New Unimplemented Device Detected");
            break;
    };

    uint32_t capabilities =
        WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(self->d_state->Core.m_seat,
                              capabilities);
}

void InputManager::init(WLR_State* state) {
    Log(Debug, "Initiaizing the input manager");
    d_state = state;

    m_newInputDeviceListener.notify = newInputDeviceResponder;

    wl_signal_add(&state->Core.m_backend->events.new_input,
                  &m_newInputDeviceListener);
}
