#include "gui/layout_presets.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>

#include <imgui.h>
#include <imgui_internal.h>

#include "gui/dockable_windows_manager.h"
#include "gui/window_chrome.h"
#include "logger/logger.h"

namespace gui::layout {
    namespace {
        bool writeTextFile(const std::string& path, const std::string& text) {
            const std::filesystem::path target(path);
            const std::filesystem::path temporary = target.string() + ".tmp";
            {
                std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
                if (!out) {
                    return false;
                }
                out << text;
                if (!out) {
                    return false;
                }
            }
            std::error_code ec;
            std::filesystem::rename(temporary, target, ec);
            return !ec;
        }
    }

    Snapshot captureCurrentLayout(const DockableWindowsManager& windows) {
        Snapshot snapshot;
        snapshot.imguiVersion = IMGUI_VERSION_NUM;
        if (const char* ini = ImGui::SaveIniSettingsToMemory(nullptr)) {
            snapshot.imguiIni = ini;
        }
        snapshot.windowsJson = windows.SaveStateToString();
        snapshot.chromeJson = gui::chrome::saveStateToString();
        return snapshot;
    }

    bool applySnapshot(const Snapshot& snapshot, DockableWindowsManager& windows) {
        if (snapshot.imguiVersion != IMGUI_VERSION_NUM) {
            return false;
        }
        ImGui::LoadIniSettingsFromMemory(snapshot.imguiIni.c_str(), snapshot.imguiIni.size());
        windows.LoadStateFromString(snapshot.windowsJson);
        gui::chrome::loadStateFromString(snapshot.chromeJson);
        ImGui::MarkIniSettingsDirty();
        return true;
    }

    bool saveDefaultLayout(const std::string& path, const DockableWindowsManager& windows) {
        return writeTextFile(path, serializeSnapshot(captureCurrentLayout(windows)));
    }

    bool loadDefaultLayout(const std::string& path, Snapshot& out) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return deserializeSnapshot(text, out);
    }

    void resetToFactoryLayout(DockableWindowsManager& windows) {
        ImGui::LoadIniSettingsFromMemory("", 0);
        windows.ResetVisibilityToDefaults();
        gui::chrome::resetState();
        ImGui::MarkIniSettingsDirty();
    }

    bool applySavedLayoutOnStartup(const std::string& path, DockableWindowsManager& windows,
                                   Logger& logger) {
        Snapshot snapshot;
        if (!loadDefaultLayout(path, snapshot)) {
            logger.addLog("[APP] Default layout not found; using the restored layout");
            return false;
        }
        if (!applySnapshot(snapshot, windows)) {
            logger.addLog("[APP] Default layout skipped: saved by a different ImGui version");
            return false;
        }
        logger.addLog("[APP] Default layout applied");
        return true;
    }

}
