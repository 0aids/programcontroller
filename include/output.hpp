// All the responders to new outputs and shit.
#pragma once
#include "manager_core.hpp"
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#define static
#include <wlr/types/wlr_scene.h>
#undef static
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
};

/* Externally required:
 *      wlr_scene           (To get the current output for a frame, 
 *                              and to assign a scene an output)
 *
 *      wlr_scene_layout    (To add an output to the scene output)
 *
 *      wlr_renderer        (To notify the renderer of a new output 
 *                              that it can render to.)
 *      wlr_allocator       (For allocation of resources for something
 *                              I really dunno)
 * 
 *      wl_display          (For creating the output layout)
 *
 *      wl_backend -> events . new_output       (obvious)
 *      
 * */
class WLR_State;

class OutputManager : public IManager {
  private:
    WLR_State*  d_state;

    wl_listener m_newOutputListener;

  public:
    static void newOutputHandler(wl_listener* listener, void* data);

    void        init(WLR_State* state);
};

class WaylandOutput {
  private:
    WLR_State*  d_state;

    wl_list     m_link;
    wlr_output* m_output;
    wl_listener m_newFrameListener;
    wl_listener m_newStateListener;
    wl_listener m_destroyRequestListener;

  public:
    void static newFrameResponder(wl_listener* listener, void* data);
    void static newStateResponder(wl_listener* listener, void* data);
    void static DestroyRequestResponder(wl_listener* listener,
                                        void*        data);

    WaylandOutput(WLR_State* state, wlr_output* output);
};
