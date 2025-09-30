#pragma once
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
}

class WaylandServer;

/* Externally required:
 *      wlr_seat
 *      top_levels (For notify_enter)
 *      
 * */

class KeyboardManager {
  public:
    WaylandServer* m_parentServer;

    wl_list        m_keyboards_l;
    // Also notifiers and shit.
    wl_listener m_keyboardInputListener;

    static void keyboardInputResponder(wl_listener* listener,
                                       void*        data);

    // Not a satic method
    void newKeyboardResponder(wlr_input_device* device);

    void init(WaylandServer* parentServer);
};
