#include "windows_dockable/logs.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <format>
#include <string>
#include <vector>

#include <windows.h>
#include <shellapi.h>

#include "gui/constants.h"
#include "gui/utils.h"
#include "gui/notifications.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    namespace {
        using Clock = std::chrono::system_clock;

        std::optional<Clock::time_point> presetFrom(logs::TimePreset preset, Clock::time_point now) {
            switch (preset) {
                case logs::TimePreset::Last5Min:    return now - std::chrono::minutes(5);
                case logs::TimePreset::Last15Min:   return now - std::chrono::minutes(15);
                case logs::TimePreset::LastHour:    return now - std::chrono::hours(1);
                case logs::TimePreset::Last24Hours: return now - std::chrono::hours(24);
                case logs::TimePreset::All:
                case logs::TimePreset::Custom:      return std::nullopt;
            }
            return std::nullopt;
        }

        void formatRangeTime(std::array<char, 24>& buffer, Clock::time_point timePoint) {
            const std::time_t raw = Clock::to_time_t(timePoint);
            std::tm local{};
            localtime_s(&local, &raw);
            std::snprintf(buffer.data(), buffer.size(), "%02d.%02d.%04d - %02d:%02d:%02d",
                          local.tm_mday, local.tm_mon + 1, local.tm_year + 1900,
                          local.tm_hour, local.tm_min, local.tm_sec);
        }

        std::string joinEntries(const std::vector<std::string>& entries,
                                const std::vector<bool>& visible,
                                const std::set<size_t>* selection) {
            std::string text;
            for (size_t i = 0; i < entries.size(); ++i) {
                const bool include = selection ? selection->count(i) != 0 : visible[i];
                if (!include) {
                    continue;
                }
                text += entries[i];
                text += '\n';
            }
            return text;
        }

        void copyToClipboard(const std::string& text, size_t lines) {
            if (text.empty()) {
                return;
            }
            ImGui::SetClipboardText(text.c_str());
            notify::copied(std::format("{} log line(s)", lines));
        }
    }

    void Logs::render(Logger& logger, const ThemeManager* themeManager) {
        const auto& sem = themeManager->semantic();
        const std::vector<std::string> entries = logger.getLogs();

        // Timestamp cache: parse once per log-size change, not every frame.
        if (m_times.size() != entries.size()) {
            m_times.resize(entries.size());
            for (size_t i = 0; i < entries.size(); ++i) {
                m_times[i] = logs::parseLogTimestamp(entries[i]);
            }
        }

        std::optional<Clock::time_point> rangeFrom;
        std::optional<Clock::time_point> rangeTo;
        if (m_preset == logs::TimePreset::Custom) {
            rangeFrom = m_customFrom;
            rangeTo = m_customTo;
        } else {
            rangeFrom = presetFrom(m_preset, Clock::now());
        }
        const bool filtering = rangeFrom.has_value() || rangeTo.has_value();

        std::vector<bool> visible(entries.size(), true);
        std::vector<size_t> visibleIndices;
        for (size_t i = 0; i < entries.size(); ++i) {
            const bool shown = !filtering ||
                (m_times[i] && logs::inTimeRange(*m_times[i], rangeFrom, rangeTo));
            visible[i] = shown;
            if (shown) {
                visibleIndices.push_back(i);
            }
        }
        for (auto it = m_selected.begin(); it != m_selected.end();) {
            if (*it >= entries.size() || !visible[*it]) {
                it = m_selected.erase(it);
            } else {
                ++it;
            }
        }
        const size_t visibleCount = visibleIndices.size();

        ImGui::BeginChild("LogsHeader", ImVec2(-1.0f, 0.0f), ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);

        const char* filterHint = filtering ? "Export visible log entries" : "Export logs to a temporary text file and open it";
        if (ImGui::Button(ICON_FA_FILE_EXPORT)) {
            const std::string content = joinEntries(entries, visible, nullptr);
            auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_Logs_Temp.txt", content);
            if (tempPathOpt) {
                ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
                logger.addLog(std::format("[GUI] Log exported to {}", tempPathOpt->string()));
                gui::notify::success("Logs exported to a temporary file");
            } else {
                logger.addLog("[GUI] Failed to create temporary file for log export");
                gui::notify::error("Log export failed");
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", filterHint);

        ImGui::SameLine();

        const std::string visibleText = joinEntries(entries, visible, nullptr);
        if (ImGuiUtils::CopyButton(ICON_FA_COPY "##logs_copy_all", visibleText.c_str(),
                                   filtering ? "Copy visible log entries" : "Copy all log entries to clipboard")) {
            logger.addLog("[GUI] Logs copied to clipboard");
        }

        ImGui::SameLine();

        if (filtering) {
            ImGui::PushStyleColor(ImGuiCol_Text, sem.info);
        }
        if (ImGui::Button(ICON_FA_FILTER "##logs_filter")) {
            ImGui::OpenPopup("##logs_filter_popup");
        }
        if (filtering) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filter entries by time");

        if (filtering) {
            ImGui::SameLine();
            const char* chipText = "custom range";
            switch (m_preset) {
                case logs::TimePreset::Last5Min:    chipText = "last 5 min"; break;
                case logs::TimePreset::Last15Min:   chipText = "last 15 min"; break;
                case logs::TimePreset::LastHour:    chipText = "last hour"; break;
                case logs::TimePreset::Last24Hours: chipText = "last 24 h"; break;
                default: break;
            }
            ImGui::TextDisabled("%s · %zu / %zu", chipText, visibleCount, entries.size());
        }

        if (ImGui::BeginPopup("##logs_filter_popup")) {
            const float labelW = ImGuiUtils::DpiScale(40.0f);
            const float fieldW = ImGuiUtils::DpiScale(170.0f);

            ImGui::TextUnformatted("From");
            ImGui::SameLine(labelW);
            ImGui::SetNextItemWidth(fieldW);
            ImGui::InputTextWithHint("##from", "dd.mm.yyyy - HH:MM:SS", m_fromText.data(), m_fromText.size());
            ImGui::TextUnformatted("To");
            ImGui::SameLine(labelW);
            ImGui::SetNextItemWidth(fieldW);
            ImGui::InputTextWithHint("##to", "dd.mm.yyyy - HH:MM:SS", m_toText.data(), m_toText.size());

            ImGui::Spacing();
            ImGui::TextDisabled("Quick range");
            auto presetButton = [this](const char* label, logs::TimePreset preset) -> bool {
                if (!ImGui::Button(label)) {
                    return false;
                }
                m_preset = preset;
                m_customFrom.reset();
                m_customTo.reset();
                m_filterError.clear();
                if (preset == logs::TimePreset::All) {
                    m_fromText.fill('\0');
                    m_toText.fill('\0');
                } else {
                    const auto now = Clock::now();
                    if (const auto from = presetFrom(preset, now)) {
                        formatRangeTime(m_fromText, *from);
                    }
                    formatRangeTime(m_toText, now);
                }
                return true;
            };
            if (presetButton("All", logs::TimePreset::All)) ImGui::CloseCurrentPopup();
            ImGui::SameLine();
            presetButton("5 min", logs::TimePreset::Last5Min);
            ImGui::SameLine();
            presetButton("15 min", logs::TimePreset::Last15Min);
            ImGui::SameLine();
            presetButton("1 h", logs::TimePreset::LastHour);
            ImGui::SameLine();
            presetButton("24 h", logs::TimePreset::Last24Hours);

            if (!m_filterError.empty()) {
                ImGui::TextColored(sem.danger, "%s", m_filterError.c_str());
            }

            ImGui::Separator();
            if (ImGui::Button("Apply")) {
                const std::string fromText = m_fromText.data();
                const std::string toText = m_toText.data();
                std::optional<Clock::time_point> from;
                std::optional<Clock::time_point> to;
                bool valid = true;
                if (!fromText.empty()) {
                    from = logs::parseLogTimestamp(fromText);
                    valid = valid && from.has_value();
                }
                if (!toText.empty()) {
                    to = logs::parseLogTimestamp(toText);
                    valid = valid && to.has_value();
                }
                if (!valid) {
                    m_filterError = "Expected dd.mm.yyyy - HH:MM:SS";
                } else {
                    m_preset = logs::TimePreset::Custom;
                    m_customFrom = from;
                    m_customTo = to;
                    m_filterError.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                m_preset = logs::TimePreset::All;
                m_customFrom.reset();
                m_customTo.reset();
                m_filterError.clear();
                m_fromText.fill('\0');
                m_toText.fill('\0');
                m_selected.clear();
                m_hasSelectionAnchor = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::Checkbox("Autoscroll", &m_autoscroll)) {
            m_jumpToBottom = m_autoscroll;
        }

        // The checkbox ends the line (ImGui leaves the cursor on a new one) -
        // return before placing the trash button, otherwise it lands on its
        // own row (seen on a screenshot).
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(ICON_FA_TRASH).x - ImGui::GetStyle().FramePadding.x * 2);

        ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
        if (ImGui::Button(ICON_FA_TRASH)) {
            m_confirmDelete = true;
        }
        ImGuiUtils::PopFilledButton();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear all log entries (requires confirmation)");
        ImGui::EndChild();

        ImGui::BeginChild("LogsContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);

        // Stick-to-bottom: capture the decision BEFORE rendering, before new
        // content shifts the scroll. Works after clearLogs() (empty list, max
        // scroll 0 counts as "at the bottom") and when the user scrolled up
        // to read history - then autoscroll stays silent until they return.
        const float bottomSlack = ImGuiUtils::DpiScale(LOGS_AUTOSCROLL_SLACK);
        const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - bottomSlack;

        auto copySelection = [&]() {
            copyToClipboard(joinEntries(entries, visible, &m_selected), m_selected.size());
        };
        auto clearSelection = [&]() {
            m_selected.clear();
            m_hasSelectionAnchor = false;
        };
        auto selectRange = [&](size_t anchorIndex, size_t targetIndex) {
            const auto anchorIt = std::find(visibleIndices.begin(), visibleIndices.end(), anchorIndex);
            const auto targetIt = std::find(visibleIndices.begin(), visibleIndices.end(), targetIndex);
            if (anchorIt == visibleIndices.end() || targetIt == visibleIndices.end()) {
                m_selected.insert(targetIndex);
                return;
            }
            const size_t first = std::min<size_t>(anchorIt - visibleIndices.begin(), targetIt - visibleIndices.begin());
            const size_t last = std::max<size_t>(anchorIt - visibleIndices.begin(), targetIt - visibleIndices.begin());
            for (size_t pos = first; pos <= last; ++pos) {
                m_selected.insert(visibleIndices[pos]);
            }
        };

        for (const size_t index : visibleIndices) {
            const bool selected = m_selected.count(index) != 0;
            ImGui::PushID(static_cast<int>(index));
            // Empty label + overlay text: log lines may contain "##", which
            // ImGui would treat as a hidden suffix and cut the visible text.
            const bool clicked = ImGui::Selectable("##logrow", selected,
                                                   ImGuiSelectableFlags_AllowDoubleClick);
            const ImVec2 rowMin = ImGui::GetItemRectMin();
            const ImVec2 rowMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const float textY = rowMin.y +
                (rowMax.y - rowMin.y - ImGui::GetTextLineHeight()) * 0.5f;
            drawList->PushClipRect(rowMin, rowMax, true);
            drawList->AddText(ImVec2(rowMin.x + ImGui::GetStyle().FramePadding.x, textY),
                              ImGui::GetColorU32(ImGuiCol_Text), entries[index].c_str());
            drawList->PopClipRect();

            if (clicked) {
                const bool ctrl = ImGui::GetIO().KeyCtrl;
                const bool shift = ImGui::GetIO().KeyShift;
                if (shift && m_hasSelectionAnchor) {
                    selectRange(m_selectionAnchor, index);
                } else if (ctrl) {
                    if (selected) {
                        m_selected.erase(index);
                    } else {
                        m_selected.insert(index);
                    }
                    m_selectionAnchor = index;
                    m_hasSelectionAnchor = true;
                } else {
                    m_selected.clear();
                    m_selected.insert(index);
                    m_selectionAnchor = index;
                    m_hasSelectionAnchor = true;
                }
            }
            if (ImGui::BeginPopupContextItem("##logs_row_ctx")) {
                if (ImGui::MenuItem("Copy selection", nullptr, false, !m_selected.empty())) {
                    copySelection();
                }
                if (ImGui::MenuItem("Copy all visible")) {
                    copyToClipboard(visibleText, visibleCount);
                }
                if (ImGui::MenuItem("Clear selection", nullptr, false, !m_selected.empty())) {
                    clearSelection();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        if (visibleCount == 0) {
            ImGui::TextDisabled("No entries match the filter");
            if (filtering) {
                if (ImGui::Button("Clear filter")) {
                    m_preset = logs::TimePreset::All;
                    m_customFrom.reset();
                    m_customTo.reset();
                    m_fromText.fill('\0');
                    m_toText.fill('\0');
                    clearSelection();
                }
            }
        }

        if (!m_selected.empty() &&
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
            copySelection();
        }

        if (m_jumpToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_jumpToBottom = false;
        } else if (m_autoscroll && wasAtBottom) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        // Confirmation modal for the header "Clear" button. The in-memory log
        // and the log files on disk are different things, so it does not reuse
        // CleanLogsConfirmDialog (that one deletes folder files from Preferences).
        // The popup is owned by the root Logs window: open it here (the same
        // window that renders the modal), not inside the header child.
        if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_CONFIRM_POPUP_NAME)) {
            ImGui::OpenPopup(DELETE_LOG_CONFIRM_POPUP_NAME);
        }
        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(
            DELETE_LOG_CONFIRM_POPUP_MIN_SIZE.x, DELETE_LOG_CONFIRM_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(DELETE_LOG_CONFIRM_POPUP_DEFAULT_SIZE);
        if (ImGuiUtils::BeginPopupModalEx(DELETE_LOG_CONFIRM_POPUP_NAME, nullptr,
                                          ImGuiWindowFlags_NoCollapse)) {
            ImGui::BeginChild("##delete_log_confirm_body",
                              ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                              ImGuiChildFlags_Borders);
            ImGui::TextUnformatted("This will clear all entries in the on-screen log.");
            ImGui::TextUnformatted("Log files on disk are not removed.");
            ImGui::EndChild();

            const float clearBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            const float cancelBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
            const float totalW = cancelBtnW + clearBtnW + ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalW) * 0.5f);

            if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(cancelBtnW, 0))) {
                m_confirmDelete = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover,
                                         sem.dangerButtonActive, sem.onAccent);
            if (ImGui::Button(ICON_FA_TRASH " Clear", ImVec2(clearBtnW, 0))) {
                logger.clearLogs();
                clearSelection();
                m_confirmDelete = false;
                ImGui::CloseCurrentPopup();
                gui::notify::success("Log entries cleared");
            }
            ImGuiUtils::PopFilledButton();

            ImGui::EndPopup();
        } else if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_CONFIRM_POPUP_NAME)) {
            // Closed through the app-wide close-popup shortcut (Esc) — drop the
            // pending request so it does not reopen on the next frame.
            m_confirmDelete = false;
        }
    }
}
