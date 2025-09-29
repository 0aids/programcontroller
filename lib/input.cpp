#include "input.hpp"
#include "log.hpp"
#include "server.hpp"
#include <wayland-server-core.h>
extern "C" {
#include <wayland-util.h>
}
#include <iostream>

void InputManager::newInputDeviceResponder(wl_listener *listener, void *data) {
  InputManager *self =
      wl_container_of(listener, self, m_newInputDeviceListener);

  Log(Debug, "New input detected");
  auto *device = static_cast<struct wlr_input_device *>(data);
  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    self->m_keyboardManager.newKeyboardResponder(device);

    break;
  case WLR_INPUT_DEVICE_POINTER:
    Log(Debug, "New pointer detected");
    wlr_cursor_attach_input_device(self->m_cursorManager.m_cursor, device);
    self->m_cursorManager.m_device = device;
    break;
  default:
    break;
  };

  uint32_t capabilities =
      WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;
  wlr_seat_set_capabilities(self->m_seat, capabilities);
}

void InputManager::init(WaylandServer *parentServer) {
  m_parentServer = parentServer;

  m_seat = wlr_seat_create(m_parentServer->m_display, "seat1");

  m_cursorManager.init(parentServer);

  m_keyboardManager.init(parentServer);

  m_newInputDeviceListener.notify = newInputDeviceResponder;

  wl_signal_add(&m_parentServer->m_backend->events.new_input,
                &m_newInputDeviceListener);
}
