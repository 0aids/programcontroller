// All the responders to new outputs and shit.
#pragma once
#include <wayland-server-core.h>
#include <wayland-util.h>
extern "C" {
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
};

class WaylandServer;

class OutputManager {
public:
  WaylandServer *m_parentServer;
  wl_listener m_newOutputListener;
  wlr_output_layout *m_outputLayout;
  wl_list m_outputs_l;

  static void newOutputHandler(wl_listener *listener, void *data);
  void init(WaylandServer *parentServer, wl_display *display);
};

class WaylandOutput {
public:
  wl_list m_link;
  WaylandServer *m_parentServer;
  wlr_output *m_output;
  wl_listener m_newFrameListener;
  wl_listener m_newStateListener;
  wl_listener m_destroyRequestListener;

public:
  void static newFrameResponder(wl_listener *listener, void *data);
  void static newStateResponder(wl_listener *listener, void *data);
  void static DestroyRequestResponder(wl_listener *listener, void *data);

  WaylandOutput(OutputManager *outputManager, wlr_output *output);
};
