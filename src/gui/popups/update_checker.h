#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#include "app/settings.h"
#include "gui/theme_manager.h"

namespace gui {
    struct UpdateInfo {
        std::string version;
        std::string releaseDate;
        std::string downloadUrl;
        bool hasUpdate = false;
    };

    class UpdateChecker {
        public:
            UpdateChecker();
            ~UpdateChecker();

            void open() noexcept;
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            void render();
            void checkForUpdates();
            std::string getCurrentVersion() const;

            void setChannel(app::UpdateChannel c) noexcept { m_channel = c; }
            void setAutoCheckOnStartup(bool b) noexcept { m_autoCheckOnStartup = b; }
            [[nodiscard]] bool isAutoCheckEnabled() const noexcept { return m_autoCheckOnStartup; }

            /// Non-owning; bound by GuiManager after graphics.initialize().
            void setThemeManager(const ThemeManager* themeManager) noexcept { m_themeManager = themeManager; }

        private:
            bool m_open = false;
            UpdateInfo m_updateInfo;
            std::string m_errorMessage;

            /// Guards m_updateInfo and m_errorMessage, which are written by the network
            /// worker thread and read by the UI thread during render(). Plain std::atom
            /// covers only the flags; the payload strings need this mutex.
            mutable std::mutex m_dataMutex;

            std::atomic<bool> m_isChecking{false};
            std::atomic<bool> m_isCheckComplete{false};

            std::thread m_workerThread;

            app::UpdateChannel m_channel{app::UpdateChannel::Stable};
            bool m_autoCheckOnStartup{false};
            const ThemeManager* m_themeManager = nullptr;

            bool fetchLatestVersion();
            int compareVersions(const std::string& v1, const std::string& v2);
            void workerThread();
            void joinThread();
    };
}