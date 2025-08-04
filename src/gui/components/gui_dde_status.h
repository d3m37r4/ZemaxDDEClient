#ifndef GUI_DDE_STATUS_H
#define GUI_DDE_STATUS_H

#include "lib/imgui/imgui.h"

namespace gui {
    // Constants defining frame size
    static constexpr float DDE_STATUS_FRAME_WIDTH = 200.0f;
    static constexpr float DDE_STATUS_FRAME_HEIGHT = 60.0f;

    // Status text color
    static constexpr ImVec4 DDE_STATUS_COLOR_CONNECTED = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);            // Green
    static constexpr ImVec4 DDE_STATUS_COLOR_DISCONNECTED = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);         // Red

    // Button colors for the "Connect" state
    static constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_NORMAL = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);       // Normal state
    static constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_HOVER = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);        // Hovered state
    static constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_ACTIVE = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);       // Active state

    // Button colors for the "Disconnect" state
    static constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_NORMAL = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);    // Normal state
    static constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_HOVER = ImVec4(0.7f, 0.0f, 0.0f, 1.0f);     // Hovered state
    static constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_ACTIVE = ImVec4(0.3f, 0.0f, 0.0f, 1.0f);    // Active state
}
#endif // GUI_DDE_STATUS_H
