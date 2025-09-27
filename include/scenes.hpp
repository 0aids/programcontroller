#pragma once
extern "C" {
#include <wlr/backend.h>
#define static
#include <wlr/types/wlr_scene.h>
#undef static
}

class WaylandServer;

class SceneManager {
public:
  WaylandServer *m_parentServer;
  wlr_scene *m_scene;
  wlr_scene_output_layout *m_sceneLayout;

  void init(WaylandServer *parentServer);
};
