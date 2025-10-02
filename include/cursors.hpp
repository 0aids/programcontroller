#pragma once
#include "manager_core.hpp"
#include <mutex>
extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
}

class WLR_State;

struct CursorConstraint {
    wlr_pointer_constraint_v1* m_constraint;
    wl_listener                m_destroyListener;

    static void destroyResponder(wl_listener* listener, void* data);
};

/* Externally required:
 *      wlr_scene -> tree . node
 *      wlr_seat
 *      wlr_seat events
 *      wl_display
 *      TopLevelsManager . m_topLevels_l (Top levels list)
 *      wlr_output_layout
 *
 * */

class CursorManager : public IManager {
  private:
    WLR_State*  d_state;

    int         sharedCounter = 0;
    std::mutex  counterMutex;
    wl_listener m_cursorMotionListener;
    wl_listener m_cursorMotionAbsoluteListener;
    wl_listener m_cursorButtonListener;
    wl_listener m_requestSetCursorListener;
    wl_listener m_newConstraintListener;

  public:
    void        cursorMotion(wlr_pointer_motion_event* event);

    static void cursorMotionResponder(wl_listener* listener,
                                      void*        data);

    static void cursorMotionAbsoluteResponder(wl_listener* listener,
                                              void*        data);

    static void cursorButtonResponder(wl_listener* listener,
                                      void*        data);

    static void requestSetCursorResponder(wl_listener* listener,
                                          void*        data);

    static void newConstraintResponder(wl_listener* listener,
                                       void*        data);

    void        init(WLR_State* state);
    void        newCursor(wlr_input_device* device);

    // wl_listener m_cursorAxisListener;
    // wl_listener m_cursorFrameListener;

    // wl_listener m_cursorListener;
    // wl_listener m_cursorSelectionListener;
};
