#pragma once
#include "manager_core.hpp"
extern "C" {
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_xdg_shell.h>
#define static
#include <wlr/types/wlr_scene.h>
#undef static
}

/* Externally required:
 *      wl_display      (for creating the xdg_shell)
 *      
 * */

class WLR_State;

class TopLevelsManager : public IManager {
  public:
    WLR_State*  d_state;

    wl_listener m_topLevelListener;
    wl_listener m_topLevelPopupListener;

  public:
    static void newTopLevelResponder(wl_listener* listener,
                                     void*        data);
    static void newTopLevelPopupResponder(wl_listener* listener,
                                          void*        data);
    void        init(WLR_State* state);
};

/* Externally required:
 *      OutputManager . wlr_output_layout       (We need to get an
 *          OR                                      output to commit to)
 *      wlr_output                          
 *
 *      CursorManager . wlr_cursor              (Finding coordinates to 
 *                                                  be focused)
 *      InputManager . wlr_seat                 (Keyboard & pointer Notify enter)
 *      
 *      SceneManager . wlr_scene . wlr_scene_tree (To create the xdg_surface)
 *      
 * */

class TopLevelView {
  public:
    WLR_State*        d_state;

    wl_list           m_link;
    wlr_xdg_toplevel* m_topLevel;
    wlr_scene_tree*   m_sceneTree;

    // NOTE: We are not using these.

    // wl_listener m_MapListener;
    // wl_listener m_UnmapListener;
    // wl_listener m_DestroyListener;
    wl_listener m_commitListener;
    int         fullscreenAttempts             = 0;
    int         maximumFullscreenAttemptAmount = 10;

  public:
    static void commitResponder(wl_listener* listener, void* data);

    TopLevelView(WLR_State* state, wlr_xdg_toplevel* topLevel);
};
