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
    U,
    F1,
    F6,
    Slash,
    Escape,
    Digit1,
    Digit2,
};

enum Mod : int {
    Mod_None = 0,
    Mod_Ctrl = 1 << 0,
    Mod_Alt = 1 << 1,
    Mod_Shift = 1 << 2,
};

struct Entry {
    const char* id;     // stable id, e.g. "open-zmx"
    Key key;            // main key
    int mods;           // Mod bitmask
    const char* hint;   // menu/tooltip text, e.g. "Ctrl+O"
    const char* action; // human-readable action for the help dialog
};

inline constexpr Entry kEntries[] = {
    {"open-zmx", Key::O, Mod_Ctrl, "Ctrl+O", "Open *.ZMX file in Zemax"},
    {"preferences", Key::Comma, Mod_Ctrl, "Ctrl+,", "Open Preferences"},
    {"check-updates", Key::U, Mod_Ctrl, "Ctrl+U", "Check for Updates"},
    {"dde-connect", Key::C, Mod_Ctrl | Mod_Shift, "Ctrl+Shift+C", "Connect to Zemax..."},
    {"dde-disconnect", Key::D, Mod_Ctrl | Mod_Shift, "Ctrl+Shift+D", "Disconnect active DDE slot"},
    {"target-next", Key::F6, Mod_None, "F6", "Switch to next DDE target"},
    {"target-slot-0", Key::Digit1, Mod_Alt, "Alt+1", "Select DDE target [0]"},
    {"target-slot-1", Key::Digit2, Mod_Alt, "Alt+2", "Select DDE target [1]"},
    {"shortcuts-help", Key::F1, Mod_None, "F1", "Keyboard shortcuts help"},
    {"shortcuts-help-alt", Key::Slash, Mod_Ctrl, "Ctrl+/", "Keyboard shortcuts help"},
    {"close-popup", Key::Escape, Mod_None, "Esc", "Close topmost dialog"},
};

inline constexpr int kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

} // namespace gui::shortcuts
