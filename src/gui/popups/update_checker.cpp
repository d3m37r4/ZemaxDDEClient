#include "gui/popups/update_checker.h"
#include "lib/imgui/imgui.h"
#include "version.h"
#include "app/app.h"
#include <nlohmann/json.hpp>
#include <winhttp.h>
#include <windows.h>
#include <string>
#include <format>
#include <cstdio>

namespace gui {
    UpdateChecker::UpdateChecker() = default;
    UpdateChecker::~UpdateChecker() = default;

    std::string UpdateChecker::getCurrentVersion() const {
        return APP_FULL_VERSION;
    }

    std::string UpdateChecker::getCurrentBuildDate() const {
        return __DATE__;
    }

    int UpdateChecker::compareVersions(const std::string& v1, const std::string& v2) {
        std::string s1 = v1;
        std::string s2 = v2;
        if (!s1.empty() && s1[0] == 'v') s1 = s1.substr(1);
        if (!s2.empty() && s2[0] == 'v') s2 = s2.substr(1);

        int m1 = 0, mi1 = 0, p1 = 0;
        int m2 = 0, mi2 = 0, p2 = 0;
        std::sscanf(s1.c_str(), "%d.%d.%d", &m1, &mi1, &p1);
        std::sscanf(s2.c_str(), "%d.%d.%d", &m2, &mi2, &p2);

        if (m1 != m2) return m1 - m2;
        if (mi1 != mi2) return mi1 - mi2;
        return p1 - p2;
    }

    bool UpdateChecker::fetchLatestVersion() {
        HINTERNET hSession = WinHttpOpen(L"ZemaxDDEClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/repos/d3m37r4/ZemaxDDEClient/releases/latest", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0);
        bool success = WinHttpReceiveResponse(hRequest, nullptr);

        std::string response;
        if (success) {
            char buffer[4096];
            DWORD dwSize = sizeof(buffer);
            while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &dwSize) && dwSize > 0) {
                buffer[dwSize] = '\0';
                response += buffer;
                dwSize = sizeof(buffer);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (!success || response.empty()) {
            m_errorMessage = "Failed to connect to GitHub API";
            return false;
        }

        try {
            auto json = nlohmann::json::parse(response);
            m_updateInfo.version = json.value("tag_name", "");
            m_updateInfo.releaseDate = json.value("published_at", "");
            m_updateInfo.downloadUrl = json.value("html_url", "");
            return true;
        } catch (...) {
            m_errorMessage = "Failed to parse GitHub response";
            return false;
        }
    }

    void UpdateChecker::checkForUpdates() {
        m_isChecking = true;
        m_isCheckComplete = false;

        if (fetchLatestVersion()) {
            int cmp = compareVersions(getCurrentVersion(), m_updateInfo.version);
            m_updateInfo.hasUpdate = cmp < 0;
        } else {
            m_updateInfo.hasUpdate = false;
        }

        m_isChecking = false;
        m_isCheckComplete = true;
    }

    void UpdateChecker::renderPopup(bool& showPopup) {
        if (showPopup) {
            ImGui::OpenPopup("Check for Updates");
            showPopup = false;
        }

        if (ImGui::BeginPopupModal("Check for Updates", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(APP_NAME);
            ImGui::Separator();
            ImGui::Spacing();

            std::string currentVer = std::format("Current version: {}", getCurrentVersion());
            ImGui::TextUnformatted(currentVer.c_str());

            ImGui::Spacing();

            if (!m_isCheckComplete) {
                ImGui::TextUnformatted("Click the button below to check for updates.");
            } else {
                if (!m_errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", m_errorMessage.c_str());
                } else if (m_updateInfo.hasUpdate) {
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "New version available: %s", m_updateInfo.version.c_str());
                    ImGui::TextUnformatted(std::format("Released: {}", m_updateInfo.releaseDate).c_str());
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Your software is up to date!");
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float windowWidth = ImGui::GetWindowSize().x;

            if (m_isChecking) {
                ImGui::TextUnformatted("Checking for updates...");
            } else {
                if (!m_updateInfo.hasUpdate || !m_isCheckComplete) {
                    ImGui::SetCursorPosX((windowWidth - 180.0f) * 0.5f);
                    if (ImGui::Button("Check for Updates", ImVec2(180, 0))) {
                        checkForUpdates();
                    }
                }

                if (m_updateInfo.hasUpdate && m_isCheckComplete) {
                    ImGui::SetCursorPosX((windowWidth - 180.0f) * 0.5f);
                    if (ImGui::Button("Download Update", ImVec2(180, 0))) {
                        if (!m_updateInfo.downloadUrl.empty()) {
                            ShellExecuteA(nullptr, "open", m_updateInfo.downloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        }
                    }
                }
            }

            ImGui::Spacing();

            ImGui::SetCursorPosX((windowWidth - 120.0f) * 0.5f);
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                m_isCheckComplete = false;
                m_updateInfo.hasUpdate = false;
                m_errorMessage.clear();
            }

            ImGui::EndPopup();
        }
    }
}