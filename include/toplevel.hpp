#pragma once
extern "C" {
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_xdg_shell.h>
#define static
#include <wlr/types/wlr_scene.h>
#undef static
}

class WaylandServer;

/* Externally required:
 *      wl_display      (for creating the xdg_shell)
 *      
 * */

class TopLevelsManager {
  public:
    WaylandServer* m_parentServer;

    wl_list        m_topLevels_l;
    wl_listener    m_topLevelListener;
    wl_listener    m_topLevelPopupListener;

    wlr_xdg_shell* m_xdgShell;

  public:
    static void newTopLevelResponder(wl_listener* listener,
                                     void*        data);
    static void newTopLevelPopupResponder(wl_listener* listener,
                                          void*        data);
    void        init(WaylandServer* parentServer);
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
    wl_list           m_link;
    WaylandServer*    m_parentServer;
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

    TopLevelView(WaylandServer*    parentServer,
                 wlr_xdg_toplevel* topLevel);
};
