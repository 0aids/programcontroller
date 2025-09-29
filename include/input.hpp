#pragma once
#include "inputs/cursors.hpp"
#include "inputs/keyboards.hpp"

extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
}

class WaylandServer;

class InputManager {
  public:
    WaylandServer*  m_parentServer;
    wlr_seat*       m_seat;
    CursorManager   m_cursorManager;
    KeyboardManager m_keyboardManager;

    wl_listener     m_newInputDeviceListener;

    static void     newInputDeviceResponder(wl_listener* listener, void* data);

    void            init(WaylandServer* parentServer);
};
