#include "input.hpp"
#include "server.hpp"
#include "output.hpp"
#include "toplevel.hpp"
#include "log.hpp"
#include "cursors.hpp"
#include "keyboards.hpp"
#include <functional>
#include "managers.hpp"

void Managers::init(WLR_State* state) {
    Log(Debug, "Initializing the managers");
    d_state = state;
    m_outputMgr.init(d_state);
    m_topLevelsMgr.init(d_state);
    m_cursorMgr.init(d_state); //  These 2 need to hook into the
    m_keyboardMgr.init(        //  inputmgr to let it know what to do.
        d_state);

    WLR_newInputFunc newCursorFunction =
        [this](wlr_input_device* device) {
            m_cursorMgr.newCursor(device);
        };
    WLR_newInputFunc newKeyboardFunction =
        [this](wlr_input_device* device) {
            m_keyboardMgr.newKeyboard(device);
        };

    m_inputMgr.setCursorResponder(newCursorFunction);
    m_inputMgr.setKeyboardResponder(newKeyboardFunction);
    m_inputMgr.init(d_state);
}

void InputManager::setCursorResponder(
    WLR_newInputFunc newCursorResponder) {
    m_newCursorResponder = newCursorResponder;
}
void InputManager::setKeyboardResponder(
    WLR_newInputFunc newKeyboardResponder) {
    m_newKeyboardResponder = newKeyboardResponder;
}
Managers::~Managers() {}
