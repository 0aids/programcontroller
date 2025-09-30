// All the responders to new outputs and shit.
#pragma once
#include <wayland-server-core.h>
#include <wayland-util.h>
extern "C" {
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
};
class WaylandServer;

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

class OutputManager {
  public:
    friend class WaylandServer;

    WaylandServer*     m_parentServer;
    wl_listener        m_newOutputListener;
    wlr_output_layout* m_outputLayout;
    wl_list            m_outputs_l;

    static void newOutputHandler(wl_listener* listener, void* data);
    void init(WaylandServer* parentServer, wl_display* display);
};

class WaylandOutput {
  public:
    wl_list        m_link;
    WaylandServer* m_parentServer;
    wlr_output*    m_output;
    wl_listener    m_newFrameListener;
    wl_listener    m_newStateListener;
    wl_listener    m_destroyRequestListener;

  public:
    void static newFrameResponder(wl_listener* listener, void* data);
    void static newStateResponder(wl_listener* listener, void* data);
    void static DestroyRequestResponder(wl_listener* listener,
                                        void*        data);

    WaylandOutput(OutputManager* outputManager, wlr_output* output);
};
