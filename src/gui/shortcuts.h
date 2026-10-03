#pragma once

// Central registry of global keyboard shortcuts.
//
// This header is intentionally ImGui-free: it only *describes* chords
// (key + modifiers + hint strings) so the table stays unit-testable
// without an ImGui context. Actual polling/dispatch lives in
// shortcuts.cpp (GuiManager::handleShortcuts).

namespace gui::shortcuts {

enum class Key : int {
    O,
    Comma,
    C,
    D,
    I,
    U,
    F1,
    F6,
    Slash,
    Escape,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
};

enum Mod : int {
    Mod_None = 0,
    Mod_Ctrl = 1 << 0,
    Mod_Alt = 1 << 1,
    Mod_Shift = 1 << 2,
};

struct Entry {
    const char* id;      // stable id, e.g. "open-zmx"
    const char* section; // help-dialog group: File, Info, DDE, Dialogs
    Key key;             // main key
    int mods;            // Mod bitmask
    const char* hint;    // menu/tooltip text, e.g. "Ctrl+O"
    const char* action;  // human-readable action for the help dialog
};

inline constexpr Entry kEntries[] = {
    {"open-zmx", "File", Key::O, Mod_Ctrl, "Ctrl+O", "Open *.ZMX file in Zemax"},
    {"preferences", "File", Key::Comma, Mod_Ctrl, "Ctrl+,", "Open Preferences"},
    {"view-dde-status", "View", Key::Digit1, Mod_Ctrl, "Ctrl+1", "Toggle DDE Status panel"},
    {"view-system-info", "View", Key::Digit2, Mod_Ctrl, "Ctrl+2", "Toggle Optical System Information panel"},
    {"view-profile", "View", Key::Digit3, Mod_Ctrl, "Ctrl+3", "Toggle Surface Profile Inspector panel"},
    {"view-map", "View", Key::Digit4, Mod_Ctrl, "Ctrl+4", "Toggle Surface Irregularity Map panel"},
    {"view-logs", "View", Key::Digit5, Mod_Ctrl, "Ctrl+5", "Toggle Logs panel"},
    {"check-updates", "Info", Key::U, Mod_Ctrl, "Ctrl+U", "Check for Updates"},
    {"about", "Info", Key::I, Mod_Ctrl, "Ctrl+I", "Open About dialog"},
    {"shortcuts-help", "Info", Key::F1, Mod_None, "F1", "Keyboard shortcuts help"},
    {"shortcuts-help-alt", "Info", Key::Slash, Mod_Ctrl, "Ctrl+/", "Keyboard shortcuts help"},
    {"dde-connect", "DDE", Key::C, Mod_Ctrl | Mod_Shift, "Ctrl+Shift+C", "Connect to Zemax..."},
    {"dde-disconnect", "DDE", Key::D, Mod_Ctrl | Mod_Shift, "Ctrl+Shift+D", "Disconnect active DDE slot"},
    {"target-next", "DDE", Key::F6, Mod_None, "F6", "Switch to next DDE target"},
    {"target-slot-0", "DDE", Key::Digit1, Mod_Alt, "Alt+1", "Select DDE target [0]"},
    {"target-slot-1", "DDE", Key::Digit2, Mod_Alt, "Alt+2", "Select DDE target [1]"},
    {"close-popup", "Dialogs", Key::Escape, Mod_None, "Esc", "Close topmost dialog"},
};

inline constexpr int kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

} // namespace gui::shortcuts
