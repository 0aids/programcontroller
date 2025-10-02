#pragma once

extern "C" {
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_xcursor_manager.h>
#define static
#include <wlr/types/wlr_scene.h>
#undef static
}
#include "input.hpp"
#include "output.hpp"
#include "toplevel.hpp"
#include "cursors.hpp"
#include "keyboards.hpp"

class Managers;

class WLR_State {
  public:
    struct Core {
        wl_display*              m_display;
        wlr_backend*             m_backend;
        wlr_renderer*            m_renderer;
        wlr_allocator*           m_allocator;
        wl_event_loop*           m_eventLoop;
        wlr_seat*                m_seat;

        wlr_compositor*          m_compositor;
        wlr_subcompositor*       m_subCompositor;
        wlr_data_device_manager* m_dataDeviceManager;

      public:
        void init();
    } Core;

    struct Outputs {
        wlr_output_layout* m_outputLayout;
        wl_list            m_outputs_l;

      public:
        void init(WLR_State& state);
    } Outputs;

    struct Scenes {
        wlr_scene*               m_scene;
        wlr_scene_output_layout* m_sceneLayout;

      public:
        void init(WLR_State& state);
    } Scenes;

    struct TopLevels {
        wlr_xdg_shell* m_xdgShell;
        wl_list        m_topLevels_l;

      public:
        void init(WLR_State& state);
    } TopLevels;

    struct Cursors {
        wlr_cursor*                      m_cursor;
        wlr_xcursor_manager*             m_xcursorManager;
        wlr_surface*                     m_focusedSurface = nullptr;
        wlr_relative_pointer_manager_v1* m_relativePointerManager;
        wlr_pointer_constraints_v1*      m_pointerConstraints;
        wlr_input_device*                m_cursorDevice;

      public:
        void init(WLR_State& state);
    } Cursors;

    struct Keyboards {
        wl_list m_keyboards_l;

      public:
        void init(WLR_State& state);
    } Keyboards;

    struct UserState {
        wlr_surface*  m_keyboardFocusedSurface;
        wlr_surface*  m_cursorFocusedSurface;
        TopLevelView* m_keyboardFocusedTopLevel;
        TopLevelView* m_cursorFocusedTopLevel;
        wlr_output*   m_focusedOutput;
    } UserState;

    const char* m_waylandSocket;

    Managers*   m_managers;

  public:
    // WLR_State();
    ~WLR_State();
    // Add a couple of helper methods to quickly get required states.

    void init();
    void start();
    void loop();
};

// If we can stack allocate we might as well. The global state will always last longer.
// inline std::unique_ptr<WLR_State> g_pWLR_State;
