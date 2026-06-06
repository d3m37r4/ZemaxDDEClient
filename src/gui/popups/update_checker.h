#pragma once

#include <string>
#include <optional>

#include "app/settings.h"

namespace gui {
    class ThemeManager;

    struct UpdateInfo {
        std::string version;
        std::string releaseDate;
        std::string downloadUrl;
        bool hasUpdate;
    };

    class UpdateChecker {
        public:
            UpdateChecker();
            ~UpdateChecker();

            void checkForUpdates();
            void renderPopup(bool& showPopup);
            UpdateInfo getUpdateInfo() const { return m_updateInfo; }
            std::string getCurrentVersion() const;
            std::string getCurrentBuildDate() const;

            void setChannel(app::UpdateChannel c) noexcept { m_channel = c; }
            void setAutoCheckOnStartup(bool b) noexcept { m_autoCheckOnStartup = b; }
            [[nodiscard]] bool isAutoCheckEnabled() const noexcept { return m_autoCheckOnStartup; }
            [[nodiscard]] bool hasChecked() const noexcept { return m_isCheckComplete; }

            /// Non-owning; bound by GuiManager after graphics.initialize().
            void setThemeManager(const ThemeManager* themeManager) noexcept { m_themeManager = themeManager; }

        private:
            UpdateInfo m_updateInfo;
            bool m_isChecking{false};
            bool m_isCheckComplete{false};
            std::string m_errorMessage;
            app::UpdateChannel m_channel{app::UpdateChannel::Stable};
            bool m_autoCheckOnStartup{false};
            const ThemeManager* m_themeManager = nullptr;

            bool fetchLatestVersion();
            int compareVersions(const std::string& v1, const std::string& v2);
    };
}