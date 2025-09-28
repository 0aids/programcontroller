#pragma once
extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
}

class WaylandServer;

struct CursorConstraint {
  wlr_pointer_constraint_v1 *m_constraint;
  wl_listener m_destroyListener;

  static void destroyResponder(wl_listener *listener, void *data);
};

class CursorManager {
public:
  WaylandServer *m_parentServer;
  wlr_cursor *m_cursor;
  wlr_xcursor_manager *m_xcursorManager;
  wlr_surface *m_focusedSurface = nullptr;

  wlr_input_device *m_device;
  wlr_relative_pointer_manager_v1 *m_relativePointerManager;
  wl_listener m_cursorMotionListener;
  wl_listener m_cursorMotionAbsoluteListener;
  wl_listener m_cursorButtonListener;
  wl_listener m_requestSetCursorListener;

  wlr_pointer_constraints_v1 *m_pointerConstraints;
  wl_listener m_newConstraintListener;

  static void cursorMotionResponder(wl_listener *listener, void *data);
  static void cursorMotionAbsoluteResponder(wl_listener *listener, void *data);
  static void cursorButtonResponder(wl_listener *listener, void *data);
  static void requestSetCursorResponder(wl_listener *listener, void *data);

  static void newConstraintResponder(wl_listener *listener, void *data);

  void init(WaylandServer *parentServer);

  // wl_listener m_cursorAxisListener;
  // wl_listener m_cursorFrameListener;

  // wl_listener m_cursorListener;
  // wl_listener m_cursorSelectionListener;
};
