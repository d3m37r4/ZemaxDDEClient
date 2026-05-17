#pragma once

#include <string>
#include <optional>

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

        private:
            UpdateInfo m_updateInfo;
            bool m_isChecking{false};
            bool m_isCheckComplete{false};
            std::string m_errorMessage;

            bool fetchLatestVersion();
            int compareVersions(const std::string& v1, const std::string& v2);
    };
}