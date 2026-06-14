#include "gui/popups/preferences_dialog.h"

#include <algorithm>
#include <cstring>

#include "app/config_path.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/settings_manager.h"
#include "lib/imgui/imgui.h"

namespace gui {

    PreferencesDialog::PreferencesDialog(SettingsManager& settings) noexcept
        : m_settings(settings) {}

    void PreferencesDialog::open() noexcept {
        m_working = m_settings.current();
        m_loaded  = m_settings.current();
        m_section = Section::General;
        m_confirmReset = false;
        m_open = true;
    }

    void PreferencesDialog::close() noexcept {
        m_open = false;
        m_confirmReset = false;
    }

    void PreferencesDialog::render() {
        if (m_open && !ImGui::IsPopupOpen(PREFERENCES_POPUP_NAME)) {
            ImGui::OpenPopup(PREFERENCES_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();

        ImGuiUtils::SetDpiScaledWindowConstraints(PREFERENCES_WINDOW_MIN_SIZE.x, PREFERENCES_WINDOW_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(PREFERENCES_WINDOW_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal(PREFERENCES_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        static const char* sidebarLabels[] = {
            "General", "Appearance", "DDE Connection", "DDE Performance", "Plot", "Updates", "Files"
        };
        if (m_sidebarWidth <= 0.0f) {
            float maxW = 0.0f;
            for (auto* l : sidebarLabels)
                maxW = std::max(maxW, ImGui::CalcTextSize(l).x);
            m_sidebarWidth = maxW + ImGui::GetStyle().FramePadding.x * 2.0f + 20.0f;
        }

        const float footerH    = ImGui::GetFrameHeightWithSpacing();
        const float availH     = ImGui::GetContentRegionAvail().y - footerH;
        const float availW     = ImGui::GetContentRegionAvail().x;
        const float splitterW  = 4.0f;

        ImGui::BeginChild("##prefs_sidebar",
                          ImVec2(m_sidebarWidth, availH),
                          ImGuiChildFlags_Borders);
        renderSidebar();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGuiUtils::SplitterH("##prefs_splitter", m_sidebarWidth, availH,
                               splitterW, 60.0f, availW - 100.0f - splitterW);

        ImGui::SameLine();

        ImGui::BeginChild("##prefs_content", ImVec2(0, availH), ImGuiChildFlags_Borders);
        renderContent();
        ImGui::EndChild();

        renderFooter();

        renderResetConfirm();

        ImGui::EndPopup();
    }

    void PreferencesDialog::renderSidebar() {
        static const char* labels[static_cast<int>(Section::Count)] = {
            "General",
            "Appearance",
            "DDE Connection",
            "DDE Performance",
            "Plot",
            "Updates",
            "Files",
        };

        for (int i = 0; i < static_cast<int>(Section::Count); ++i) {
            const bool selected = (static_cast<int>(m_section) == i);
            if (ImGui::Selectable(labels[i], selected)) {
                m_section = static_cast<Section>(i);
            }
        }
    }

    void PreferencesDialog::renderContent() {
        switch (m_section) {
            case Section::General:       renderSectionGeneral();       break;
            case Section::Appearance:    renderSectionAppearance();    break;
            case Section::DDE:           renderSectionDDE();           break;
            case Section::DDEPerformance: renderSectionDDEPerformance(); break;
            case Section::Plot:          renderSectionPlot();          break;
            case Section::Updates:       renderSectionUpdates();       break;
            case Section::Files:         renderSectionFiles();         break;
            default: break;
        }
    }

    void PreferencesDialog::renderSectionGeneral() {
        ImGuiUtils::SectionHeader("General", "These options take effect on the next application start.");

        ImGui::Checkbox("Show debug log window on startup", &m_working.general.showDebugLogOnStartup);
        ImGui::Checkbox("Restore window layout on startup", &m_working.general.restoreWindowLayout);
    }

    void PreferencesDialog::renderSectionAppearance() {
        ImGuiUtils::SectionHeader("Appearance", "Theme changes are applied immediately.");

        int mode = static_cast<int>(m_working.appearance.themeMode);
        if (ImGui::RadioButton("Light",  &mode, static_cast<int>(app::ThemeMode::Light)))  { m_working.appearance.themeMode = app::ThemeMode::Light;  applyWorkingTheme(); }
        if (ImGui::RadioButton("Dark",   &mode, static_cast<int>(app::ThemeMode::Dark)))   { m_working.appearance.themeMode = app::ThemeMode::Dark;   applyWorkingTheme(); }
        if (ImGui::RadioButton("System", &mode, static_cast<int>(app::ThemeMode::System))) { m_working.appearance.themeMode = app::ThemeMode::System; applyWorkingTheme(); }
    }

    void PreferencesDialog::renderSectionDDE() {
        ImGuiUtils::SectionHeader("DDE Connection", "New values apply to subsequent requests only.");

        if (ImGui::SliderInt("Connection timeout (ms)", &m_working.dde.connectionTimeoutMs, 1000, 30000, "%d", ImGuiSliderFlags_AlwaysClamp)) {
            applyWorkingDDE();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maximum time to wait for a single DDE round-trip.");
        }

        if (ImGui::SliderInt("Max retry count", &m_working.dde.maxRetryCount, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp)) {
            applyWorkingDDE();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("How many times to retry a failed DDE exchange.");
        }

        if (ImGui::SliderInt("Max concurrent connections", &m_working.dde.maxConnections, 1, 16, "%d", ImGuiSliderFlags_AlwaysClamp)) {
            applyWorkingDDE();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Upper bound on parallel DDE clients kept open by the manager.");
        }

        if (ImGui::Checkbox("Auto-reconnect on drop", &m_working.dde.autoReconnect)) {
            applyWorkingDDE();
        }
    }

    void PreferencesDialog::renderSectionDDEPerformance() {
        ImGuiUtils::SectionHeader("DDE Performance", "Per-request timeout overrides (ms).");

        auto inputMs = [](const char* label, const char* id, int* value) {
            ImGui::TextUnformatted(label);
            ImGui::SameLine(200);
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt(id, value, 0, 0);
            ImGui::SameLine();
            ImGui::TextUnformatted("ms");
        };

        // --- Optical System Info ---
        if (ImGui::BeginChild("##perf_initdata", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 7), ImGuiChildFlags_Borders)) {
            ImGui::TextUnformatted("Optical System Info");
            ImGui::Spacing();
            inputMs("GetName",        "##t GetName",        &m_working.dde.getNameTimeoutMs);
            inputMs("GetFile",        "##t GetFile",        &m_working.dde.getFileTimeoutMs);
            inputMs("GetSystem",      "##t GetSystem",      &m_working.dde.getSystemTimeoutMs);
            inputMs("GetField",       "##t GetField",       &m_working.dde.getFieldTimeoutMs);
            inputMs("GetWave",        "##t GetWave",        &m_working.dde.getWaveTimeoutMs);
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // --- Surface Profile Inspector ---
        if (ImGui::BeginChild("##perf_profile", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 4), ImGuiChildFlags_Borders)) {
            ImGui::TextUnformatted("Surface Profile Inspector");
            ImGui::Spacing();
            inputMs("GetSurfaceData", "##t P GetSurf", &m_working.dde.getSurfaceDataProfileTimeoutMs);
            inputMs("GetSag",         "##t P GetSag",  &m_working.dde.getSagProfileTimeoutMs);
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // --- Surface Irregularity Map ---
        if (ImGui::BeginChild("##perf_map", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 4), ImGuiChildFlags_Borders)) {
            ImGui::TextUnformatted("Surface Irregularity Map");
            ImGui::Spacing();
            inputMs("GetSurfaceData", "##t M GetSurf", &m_working.dde.getSurfaceDataMapTimeoutMs);
            inputMs("GetSag",         "##t M GetSag",  &m_working.dde.getSagMapTimeoutMs);
        }
        ImGui::EndChild();

        applyWorkingDDE();
    }

    void PreferencesDialog::renderSectionPlot() {
        ImGuiUtils::SectionHeader("Plot", "Visual style for new plots.");

        if (ImGui::Checkbox("Show grid by default", &m_working.plot.showGridByDefault)) {
            applyWorkingPlot();
        }

        if (ImGui::SliderFloat("Line weight (px)", &m_working.plot.lineWeight, 0.5f, 5.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
            applyWorkingPlot();
        }

        if (ImGui::SliderFloat("Marker size (px)", &m_working.plot.markerSize, 1.0f, 20.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
            applyWorkingPlot();
        }
    }

    void PreferencesDialog::renderSectionUpdates() {
        ImGuiUtils::SectionHeader("Updates", "Update checker settings. Channel changes take effect on next check.");

        ImGui::Checkbox("Check for updates on startup", &m_working.updates.autoCheckOnStartup);

        ImGui::Spacing();
        ImGui::TextUnformatted("Update channel");
        int channel = static_cast<int>(m_working.updates.channel);
        if (ImGui::RadioButton("Stable", &channel, static_cast<int>(app::UpdateChannel::Stable))) {
            m_working.updates.channel = app::UpdateChannel::Stable;
        }
        if (ImGui::RadioButton("Beta",   &channel, static_cast<int>(app::UpdateChannel::Beta))) {
            m_working.updates.channel = app::UpdateChannel::Beta;
        }
    }

    void PreferencesDialog::renderSectionFiles() {
        ImGuiUtils::SectionHeader("Files", "Configuration file locations (read-only).");

        auto pathRow = [](const char* label, const char* id, const char* path) {
            ImGui::TextUnformatted(label);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText(id, const_cast<char*>(path),
                             static_cast<int>(std::strlen(path)), ImGuiInputTextFlags_ReadOnly);
            ImGui::Spacing();
        };

        pathRow("ImGui Layout",         "##path_imgui",    app::getImguiIniPath());
        pathRow("Window Layout",        "##path_window",   app::getWindowStatePath());
        pathRow("Application Settings", "##path_settings", app::getSettingsJsonPath());
    }

    void PreferencesDialog::renderFooter() {
        float resetBtnW   = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float cancelBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float saveBtnW    = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);

        if (ImGui::Button("Reset", ImVec2(resetBtnW, 0))) {
            m_confirmReset = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Discard all changes and restore factory defaults.");
        }

        const float groupWidth = cancelBtnW + saveBtnW + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - groupWidth);

        if (ImGui::Button("Cancel", ImVec2(cancelBtnW, 0))) {
            onCancel();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Discard all changes and revert to the last saved values.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(saveBtnW, 0))) {
            onSave();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Persist the current values to settings.json.");
        }
    }

    void PreferencesDialog::renderResetConfirm() {
        if (!m_confirmReset) return;

        ImGui::OpenPopup("Reset Preferences?");
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Reset Preferences?", &m_confirmReset,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::BeginChild("##reset_confirm_body",
                              ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                              ImGuiChildFlags_Borders);
            ImGui::TextUnformatted("This will reset all preferences to their factory defaults.");
            ImGui::TextUnformatted("Unsaved changes will be lost.");
            ImGui::EndChild();

            float cancelBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            float resetBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            const float totalW = cancelBtnW + resetBtnW + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalW) * 0.5f);
            if (ImGui::Button("Cancel", ImVec2(cancelBtnW, 0))) {
                m_confirmReset = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(resetBtnW, 0))) {
                onReset();
                m_confirmReset = false;
            }
            ImGui::EndPopup();
        }
    }

    void PreferencesDialog::applyWorkingTheme() const {
        m_settings.applyTheme(m_working.appearance);
    }

    void PreferencesDialog::applyWorkingDDE() const {
        m_settings.applyDDE(m_working.dde);
    }

    void PreferencesDialog::applyWorkingPlot() const {
        m_settings.applyPlot(m_working.plot);
    }

    void PreferencesDialog::onSave() {
        m_settings.apply(m_working);
        m_loaded = m_working;
        m_settings.saveToFile();
    }

    void PreferencesDialog::onCancel() {
        m_working = m_loaded;
        m_settings.apply(m_working);
    }

    void PreferencesDialog::onReset() {
        m_working = app::AppSettings::defaults();
        m_settings.apply(m_working);
    }

} // namespace gui
