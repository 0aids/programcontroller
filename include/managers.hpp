#include "server.hpp"
#include "output.hpp"
#include <functional>
#include <unordered_map>

class Managers {
  private:
    WLR_State* d_state;

  public:
    OutputManager    m_outputMgr;
    TopLevelsManager m_topLevelsMgr;
    InputManager     m_inputMgr;
    KeyboardManager  m_keyboardMgr;
    CursorManager    m_cursorMgr;
    void             init(WLR_State* state);
    ~Managers();
};
