#include "gui/popups/update_checker.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
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

    void UpdateChecker::open() noexcept {
        m_open = true;
    }

    void UpdateChecker::close() noexcept {
        m_open = false;
    }

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

        const wchar_t* path = (m_channel == app::UpdateChannel::Beta)
            ? L"/repos/d3m37r4/ZemaxDDEClient/releases"
            : L"/repos/d3m37r4/ZemaxDDEClient/releases/latest";

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
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

            if (m_channel == app::UpdateChannel::Beta) {
                // /releases returns an array; pick the first prerelease.
                if (!json.is_array() || json.empty()) {
                    m_errorMessage = "No releases found on Beta channel";
                    return false;
                }
                const nlohmann::json* match = nullptr;
                for (const auto& rel : json) {
                    if (rel.is_object() && rel.value("prerelease", false)) {
                        match = &rel;
                        break;
                    }
                }
                if (!match) {
                    m_errorMessage = "No prerelease found on Beta channel";
                    return false;
                }
                m_updateInfo.version     = match->value("tag_name", "");
                m_updateInfo.releaseDate = match->value("published_at", "");
                m_updateInfo.downloadUrl = match->value("html_url", "");
            } else {
                m_updateInfo.version     = json.value("tag_name", "");
                m_updateInfo.releaseDate = json.value("published_at", "");
                m_updateInfo.downloadUrl = json.value("html_url", "");
            }
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

    void UpdateChecker::render() {
        if (m_open && !ImGui::IsPopupOpen(UPDATE_POPUP_NAME)) {
            ImGui::OpenPopup(UPDATE_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();

        ImGuiUtils::SetDpiScaledWindowConstraints(UPDATE_POPUP_MIN_SIZE.x, UPDATE_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(UPDATE_POPUP_DEFAULT_SIZE);

        if (!ImGui::BeginPopupModal(UPDATE_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        if (!m_themeManager) {
            ImGui::EndPopup();
            return;
        }

        const float footerH = ImGui::GetFrameHeightWithSpacing();

        ImGui::BeginChild("##update_body", ImVec2(0, -footerH), ImGuiChildFlags_Borders);

        std::string currentVer = std::format("Current version: {}", getCurrentVersion());
        ImGui::TextUnformatted(currentVer.c_str());

        ImGui::Spacing();

        if (!m_isCheckComplete) {
            ImGui::TextUnformatted("Click the button below to check for updates.");
        } else {
            const auto& sem = m_themeManager->semantic();
            if (!m_errorMessage.empty()) {
                ImGui::TextColored(sem.danger, "Error: %s", m_errorMessage.c_str());
            } else if (m_updateInfo.hasUpdate) {
                ImGui::TextColored(sem.success, "New version available: %s", m_updateInfo.version.c_str());
                ImGui::TextUnformatted(std::format("Released: {}", m_updateInfo.releaseDate).c_str());
            } else {
                ImGui::TextColored(sem.muted, "Your software is up to date!");
            }
        }

        ImGui::EndChild();

        float windowWidth = ImGui::GetWindowSize().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float okBtnW = ImGuiUtils::DpiScale(120.0f);

        if (!m_isChecking) {
            float actionBtnW = ImGuiUtils::DpiScale(180.0f);
            float totalW = actionBtnW + spacing + okBtnW;
            ImGui::SetCursorPosX((windowWidth - totalW) * 0.5f);

            if (!m_updateInfo.hasUpdate || !m_isCheckComplete) {
                if (ImGuiUtils::SpinnerButton(UPDATE_POPUP_NAME, m_isChecking, ImVec2(actionBtnW, 0))) {
                    checkForUpdates();
                }
            } else if (m_updateInfo.hasUpdate && m_isCheckComplete) {
                if (ImGuiUtils::SpinnerButton("Download Update", false, ImVec2(actionBtnW, 0))) {
                    if (!m_updateInfo.downloadUrl.empty()) {
                        ShellExecuteA(nullptr, "open", m_updateInfo.downloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    }
                }
            }
            ImGui::SameLine();
        } else {
            ImGui::SetCursorPosX((windowWidth - okBtnW) * 0.5f);
        }

        if (ImGui::Button("OK", ImVec2(okBtnW, 0))) {
            close();
            m_isCheckComplete = false;
            m_updateInfo.hasUpdate = false;
            m_errorMessage.clear();
        }

        ImGui::EndPopup();
    }
}
