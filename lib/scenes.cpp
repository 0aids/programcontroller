#include "scenes.hpp"
#include "server.hpp"

void SceneManager::init(WaylandServer *parentServer) {
  m_parentServer = parentServer;
  m_scene = wlr_scene_create();
  m_sceneLayout = wlr_scene_attach_output_layout(
      m_scene, parentServer->m_outputManager.m_outputLayout);
}
