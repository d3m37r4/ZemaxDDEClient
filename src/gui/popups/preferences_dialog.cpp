#include "gui/popups/preferences_dialog.h"

#include <algorithm>

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
            "General", "Appearance", "DDE Connection", "Plot", "Updates"
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

        ImGui::EndPopup();

        renderResetConfirm();
    }

    void PreferencesDialog::renderSidebar() {
        static const char* labels[static_cast<int>(Section::Count)] = {
            "General",
            "Appearance",
            "DDE Connection",
            "Plot",
            "Updates",
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
            case Section::General:    renderSectionGeneral();    break;
            case Section::Appearance: renderSectionAppearance(); break;
            case Section::DDE:        renderSectionDDE();        break;
            case Section::Plot:       renderSectionPlot();       break;
            case Section::Updates:    renderSectionUpdates();    break;
            default: break;
        }
    }

    void PreferencesDialog::renderSectionGeneral() {
        ImGui::TextUnformatted("General");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("These options take effect on the next application start.");
        ImGui::Spacing();

        ImGui::Checkbox("Show debug log window on startup", &m_working.general.showDebugLogOnStartup);
        ImGui::Checkbox("Restore window layout on startup", &m_working.general.restoreWindowLayout);
    }

    void PreferencesDialog::renderSectionAppearance() {
        ImGui::TextUnformatted("Appearance");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Theme changes are applied immediately.");
        ImGui::Spacing();

        int mode = static_cast<int>(m_working.appearance.themeMode);
        if (ImGui::RadioButton("Light",  &mode, static_cast<int>(app::ThemeMode::Light)))  { m_working.appearance.themeMode = app::ThemeMode::Light;  applyWorkingTheme(); }
        if (ImGui::RadioButton("Dark",   &mode, static_cast<int>(app::ThemeMode::Dark)))   { m_working.appearance.themeMode = app::ThemeMode::Dark;   applyWorkingTheme(); }
        if (ImGui::RadioButton("System", &mode, static_cast<int>(app::ThemeMode::System))) { m_working.appearance.themeMode = app::ThemeMode::System; applyWorkingTheme(); }
    }

    void PreferencesDialog::renderSectionDDE() {
        ImGui::TextUnformatted("DDE Connection");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("New values apply to subsequent requests only.");
        ImGui::Spacing();

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

    void PreferencesDialog::renderSectionPlot() {
        ImGui::TextUnformatted("Plot");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Visual style for new plots.");
        ImGui::Spacing();

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
        ImGui::TextUnformatted("Updates");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Update checker settings. Channel changes take effect on next check.");
        ImGui::Spacing();

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

    void PreferencesDialog::renderFooter() {
        const float w = ImGuiUtils::DpiScale(120.0f);

        if (ImGui::Button("Reset", ImVec2(w, 0))) {
            m_confirmReset = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Discard all changes and restore factory defaults.");
        }

        const float groupWidth = 2.0f * w + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - groupWidth);

        if (ImGui::Button("Cancel", ImVec2(w, 0))) {
            onCancel();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Discard all changes and revert to the last saved values.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(w, 0))) {
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
            ImGui::TextUnformatted("This will reset all preferences to their factory defaults.");
            ImGui::TextUnformatted("Unsaved changes will be lost.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const float w = ImGuiUtils::DpiScale(120.0f);
            if (ImGui::Button("Cancel", ImVec2(w, 0))) {
                m_confirmReset = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(w, 0))) {
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
