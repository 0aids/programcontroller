#pragma once
#include "log.hpp"
#include "manager_core.hpp"
#include <functional>

extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
}

/* Externally required:
 *      wlr_backend -> events . new_input
 *
 * */
class WLR_State;

using WLR_newInputFunc = std::function<void(wlr_input_device*)>;
class InputManager : public IManager {
  private:
    WLR_State*       d_state;

    wl_listener      m_newInputDeviceListener;

    static void      newInputDeviceResponder(wl_listener* listener,
                                             void*        data);

    WLR_newInputFunc m_newKeyboardResponder =
        [](wlr_input_device* device) {
            Log(Warning,
                "KeyboardResponder has not been set for the "
                "InputManager");
        };
    WLR_newInputFunc m_newCursorResponder =
        [](wlr_input_device* device) {
            Log(Warning,
                "CursorResponder has not been set for the "
                "InputManager");
        };

  public:
    void init(WLR_State* state);

    void setCursorResponder(WLR_newInputFunc newCursorResponder);
    void setKeyboardResponder(WLR_newInputFunc newKeyboardResponder);
};
