#include "gui/shortcuts.h"

#include "app/app.h"
#include "gui/gui.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
namespace {

ImGuiKey toImGuiKey(shortcuts::Key key) {
    using shortcuts::Key;
    switch (key) {
        case Key::O: return ImGuiKey_O;
        case Key::Comma: return ImGuiKey_Comma;
        case Key::C: return ImGuiKey_C;
        case Key::D: return ImGuiKey_D;
        case Key::I: return ImGuiKey_I;
        case Key::U: return ImGuiKey_U;
        case Key::F1: return ImGuiKey_F1;
        case Key::F6: return ImGuiKey_F6;
        case Key::Slash: return ImGuiKey_Slash;
        case Key::Escape: return ImGuiKey_Escape;
        case Key::Digit1: return ImGuiKey_1;
        case Key::Digit2: return ImGuiKey_2;
    }
    return ImGuiKey_None;
}

bool isCtrlDown() {
    return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
}

bool isAltDown() {
    return ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
}

bool isShiftDown() {
    return ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
}

// Exact-match on Ctrl/Alt/Shift: holding extra modifiers must not trigger
// an unrelated shortcut (e.g. Ctrl+C must not fire Ctrl+Shift+C).
bool isChordPressed(shortcuts::Key key, int mods) {
    using shortcuts::Mod_Alt;
    using shortcuts::Mod_Ctrl;
    using shortcuts::Mod_Shift;
    const bool wantCtrl = (mods & Mod_Ctrl) != 0;
    const bool wantAlt = (mods & Mod_Alt) != 0;
    const bool wantShift = (mods & Mod_Shift) != 0;
    if (isCtrlDown() != wantCtrl || isAltDown() != wantAlt || isShiftDown() != wantShift) {
        return false;
    }
    return ImGui::IsKeyPressed(toImGuiKey(key), false);
}

} // namespace

bool GuiManager::isAnyModalOpen() const noexcept {
    if (m_aboutDialog && m_aboutDialog->isOpen()) {
        return true;
    }
    if (m_shortcutsHelpDialog && m_shortcutsHelpDialog->isOpen()) {
        return true;
    }
    if (m_updateChecker && m_updateChecker->isOpen()) {
        return true;
    }
    if (m_preferencesDialog && m_preferencesDialog->isOpen()) {
        return true;
    }
    if (m_connectionLostDialog && m_connectionLostDialog->isOpen()) {
        return true;
    }
    if (m_ddeStatusRenderer && m_ddeStatusRenderer->isConnectPopupOpen()) {
        return true;
    }
    return false;
}

void GuiManager::handleShortcuts() {
    // Esc owns the keyboard: it dismisses the topmost dialog even while
    // typing in one of its fields (same as the X button).
    if (isChordPressed(shortcuts::Key::Escape, shortcuts::Mod_None)) {
        closeTopmostPopup();
        return;
    }
    // Guard 1: never steal keys from text inputs (same behavior as before,
    // now centralized).
    if (ImGui::GetIO().WantTextInput) {
        return;
    }
    // Guard 2: modal dialogs own the keyboard while open.
    if (isAnyModalOpen()) {
        return;
    }

    if (isChordPressed(shortcuts::Key::O, shortcuts::Mod_Ctrl)) {
        App::openZmxFileInZemax(m_logger);
    } else if (isChordPressed(shortcuts::Key::Comma, shortcuts::Mod_Ctrl)) {
        if (m_menuBarController) {
            m_menuBarController->openPreferences();
        }
    } else if (isChordPressed(shortcuts::Key::U, shortcuts::Mod_Ctrl)) {
        openUpdates();
    } else if (isChordPressed(shortcuts::Key::I, shortcuts::Mod_Ctrl)) {
        if (m_aboutDialog) {
            m_aboutDialog->open();
        }
    } else if (isChordPressed(shortcuts::Key::C, shortcuts::Mod_Ctrl | shortcuts::Mod_Shift)) {
        openDdeConnect();
    } else if (isChordPressed(shortcuts::Key::D, shortcuts::Mod_Ctrl | shortcuts::Mod_Shift)) {
        disconnectDdeSlot();
    } else if (isChordPressed(shortcuts::Key::F6, shortcuts::Mod_None)) {
        cycleDdeTarget();
    } else if (isChordPressed(shortcuts::Key::Digit1, shortcuts::Mod_Alt)) {
        selectDdeTarget(0);
    } else if (isChordPressed(shortcuts::Key::Digit2, shortcuts::Mod_Alt)) {
        selectDdeTarget(1);
    } else if (isChordPressed(shortcuts::Key::F1, shortcuts::Mod_None)
               || isChordPressed(shortcuts::Key::Slash, shortcuts::Mod_Ctrl)) {
        openShortcutsHelp();
    }
}

void GuiManager::openDdeConnect() {
    if (!m_ddeConnectionManager || !m_ddeStatusRenderer) {
        return;
    }
    for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
        auto* conn = m_ddeConnectionManager->getConnection(i);
        if (conn && !conn->isConnected()) {
            m_ddeStatusRenderer->openConnectPopup();
            return;
        }
    }
}

void GuiManager::disconnectDdeSlot() {
    if (m_uiOpMonitor.hasActiveTasks()) {
        return;
    }
    if (!m_ddeConnectionManager) {
        return;
    }
    const int active = m_ddeConnectionManager->getActiveIndex();
    if (active < 0) {
        return;
    }
    auto* conn = m_ddeConnectionManager->getConnection(active);
    if (conn && conn->isConnected()) {
        m_ddeConnectionManager->disconnect(active);
        m_logger.addLog("[DDE] Disconnected from Zemax");
    }
}

void GuiManager::openUpdates() {
    if (m_updateChecker) {
        m_updateChecker->open();
    }
}

void GuiManager::openShortcutsHelp() {
    if (m_shortcutsHelpDialog) {
        m_shortcutsHelpDialog->open();
    }
}

void GuiManager::cycleDdeTarget() {
    if (!m_ddeConnectionManager) {
        return;
    }
    int connected[DDEConnectionManager::MAX_CONNECTIONS];
    int count = 0;
    for (int i = 0; i < DDEConnectionManager::MAX_CONNECTIONS; ++i) {
        auto* conn = m_ddeConnectionManager->getConnection(i);
        if (conn && conn->isConnected()) {
            connected[count++] = i;
        }
    }
    if (count < 2) {
        return;
    }
    const int active = m_ddeConnectionManager->getActiveIndex();
    for (int k = 0; k < count; ++k) {
        if (connected[k] == active) {
            m_ddeConnectionManager->setActiveConnection(connected[(k + 1) % count]);
            return;
        }
    }
    m_ddeConnectionManager->setActiveConnection(connected[0]);
}

void GuiManager::selectDdeTarget(int slot) {
    if (!m_ddeConnectionManager) {
        return;
    }
    if (slot < 0 || slot >= DDEConnectionManager::MAX_CONNECTIONS) {
        return;
    }
    auto* conn = m_ddeConnectionManager->getConnection(slot);
    if (conn && conn->isConnected()) {
        m_ddeConnectionManager->setActiveConnection(slot);
    }
}

bool GuiManager::closeTopmostPopup() {
    if (m_preferencesDialog && m_preferencesDialog->isOpen()) {
        return m_preferencesDialog->closeTopmost();
    }
    if (m_ddeStatusRenderer && m_ddeStatusRenderer->isConnectPopupOpen()) {
        m_ddeStatusRenderer->closeConnectPopup();
        return true;
    }
    if (m_connectionLostDialog && m_connectionLostDialog->isOpen()) {
        m_connectionLostDialog->close();
        return true;
    }
    if (m_updateChecker && m_updateChecker->isOpen()) {
        m_updateChecker->close();
        return true;
    }
    if (m_aboutDialog && m_aboutDialog->isOpen()) {
        m_aboutDialog->close();
        return true;
    }
    if (m_shortcutsHelpDialog && m_shortcutsHelpDialog->isOpen()) {
        m_shortcutsHelpDialog->close();
        return true;
    }
    return false;
}

} // namespace gui
