#pragma once
extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
}

class WaylandServer;

class CursorManager {
public:
  WaylandServer *m_parentServer;
  wlr_cursor *m_cursor;
  wlr_xcursor_manager *m_xcursorManager;

  wl_listener m_cursorMotionListener;
  wl_listener m_cursorMotionAbsoluteListener;
  wl_listener m_cursorButtonListener;

  static void cursorMotionResponder(wl_listener *listener, void *data);
  static void cursorMotionAbsoluteResponder(wl_listener *listener, void *data);
  static void cursorButtonResponder(wl_listener *listener, void *data);

  void init(WaylandServer *parentServer);

  // wl_listener m_cursorAxisListener;
  // wl_listener m_cursorFrameListener;

  // wl_listener m_cursorListener;
  // wl_listener m_cursorSelectionListener;
};
