#pragma once

#include <string>
#include <optional>

#include "app/settings.h"

namespace gui {
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

        private:
            UpdateInfo m_updateInfo;
            bool m_isChecking{false};
            bool m_isCheckComplete{false};
            std::string m_errorMessage;
            app::UpdateChannel m_channel{app::UpdateChannel::Stable};
            bool m_autoCheckOnStartup{false};

            bool fetchLatestVersion();
            int compareVersions(const std::string& v1, const std::string& v2);
    };
}