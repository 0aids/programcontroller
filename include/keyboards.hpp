#pragma once
#include "manager_core.hpp"
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
}

class WLR_State;

/* Externally required:
 *      wlr_seat
 *      top_levels (For notify_enter)
 *      
 * */

class KeyboardManager : public IManager {
  public:
    WLR_State* d_state;

    wl_list    m_keyboards_l;
    // Also notifiers and shit.
    wl_listener m_keyboardInputListener;

    static void keyboardInputResponder(wl_listener* listener,
                                       void*        data);

    void        init(WLR_State* state);
    void        newKeyboard(wlr_input_device* device);
};
