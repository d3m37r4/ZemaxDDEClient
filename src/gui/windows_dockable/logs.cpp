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

    bool Logs::cancelDeleteConfirm() noexcept {
        if (!m_confirmDelete) {
            return false;
        }
        m_confirmDelete = false;
        return true;
    }

    void Logs::render(Logger& logger, const ThemeManager* themeManager) {
        const auto& sem = themeManager->semantic();

        // Snapshot + metadata are refreshed only when the logger actually changed:
        // copying tens of thousands of strings every frame was the main reason the
        // panel dragged the whole application down after a big map run.
        const uint64_t revision = logger.revision();
        if (revision != m_cacheRevision) {
            m_entries = logger.getLogs();
            m_cache.resize(m_entries.size());
            for (size_t i = 0; i < m_entries.size(); ++i) {
                m_cache[i].time = logs::parseLogTimestamp(m_entries[i]);
                m_cache[i].level = logs::levelOf(m_entries[i]);
                m_cache[i].tone = logs::looksLikeProblem(m_entries[i]) ? RowTone::Problem
                    : logs::looksLikeSuccess(m_entries[i]) ? RowTone::Success
                                                           : RowTone::Normal;
            }
            m_cacheRevision = revision;
        }
        const std::vector<std::string>& entries = m_entries;

        std::optional<Clock::time_point> rangeFrom;
        std::optional<Clock::time_point> rangeTo;
        if (m_preset == logs::TimePreset::Custom) {
            rangeFrom = m_customFrom;
            rangeTo = m_customTo;
        } else {
            rangeFrom = presetFrom(m_preset, Clock::now());
        }
        const bool timeFiltering = rangeFrom.has_value() || rangeTo.has_value();

        const std::string_view search(m_searchText.data());
        const bool searchActive = !search.empty();
        const bool levelFiltering = !(m_showApp && m_showDde && m_showGui && m_showService && m_showOther);
        auto levelVisible = [this](logs::LogLevel level) {
            switch (level) {
                case logs::LogLevel::App:     return m_showApp;
                case logs::LogLevel::Dde:     return m_showDde;
                case logs::LogLevel::Gui:     return m_showGui;
                case logs::LogLevel::Service: return m_showService;
                case logs::LogLevel::Unknown: return m_showOther;
            }
            return true;
        };

        std::vector<bool> visible(entries.size(), true);
        std::vector<size_t> visibleIndices;
        for (size_t i = 0; i < entries.size(); ++i) {
            const bool timeOk = !timeFiltering ||
                (m_cache[i].time && logs::inTimeRange(*m_cache[i].time, rangeFrom, rangeTo));
            const bool shown = timeOk &&
                levelVisible(m_cache[i].level) &&
                (!searchActive || logs::containsNoCase(entries[i], search));
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
        const bool filtering = timeFiltering || levelFiltering || searchActive;

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

        // Build the clipboard text only on click: it used to be assembled on
        // every frame, which alone could allocate megabytes per second.
        if (ImGui::Button(ICON_FA_COPY "##logs_copy_all")) {
            copyToClipboard(joinEntries(entries, visible, nullptr), visibleCount);
            logger.addLog("[GUI] Logs copied to clipboard");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", filtering ? "Copy visible log entries"
                                              : "Copy all log entries to clipboard");
        }

        ImGui::SameLine();

        if (searchActive) {
            ImGui::PushStyleColor(ImGuiCol_Text, sem.info);
        }
        if (ImGui::Button(ICON_FA_MAGNIFYING_GLASS "##logs_search")) {
            ImGui::OpenPopup("##logs_search_popup");
        }
        if (searchActive) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Search log entries");

        if (ImGui::BeginPopup("##logs_search_popup")) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(ImGuiUtils::DpiScale(220.0f));
            ImGui::InputTextWithHint("##logs_search_text", "Search log entries...",
                                     m_searchText.data(), m_searchText.size());
            ImGui::SameLine();
            ImGui::BeginDisabled(m_searchText[0] == '\0');
            if (ImGui::Button(ICON_FA_XMARK " Clear")) {
                m_searchText.fill('\0');
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Clear the search query");
            }
            ImGui::TextDisabled("%zu / %zu entries", visibleCount, entries.size());
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const bool filterButtonActive = timeFiltering || levelFiltering;
        if (filterButtonActive) {
            ImGui::PushStyleColor(ImGuiCol_Text, sem.info);
        }
        if (ImGui::Button(ICON_FA_FILTER "##logs_filter")) {
            ImGui::OpenPopup("##logs_filter_popup");
        }
        if (filterButtonActive) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filter entries by time and level");

        if (filtering) {
            ImGui::SameLine();
            std::string chip;
            if (timeFiltering) {
                switch (m_preset) {
                    case logs::TimePreset::Last5Min:    chip = "last 5 min"; break;
                    case logs::TimePreset::Last15Min:   chip = "last 15 min"; break;
                    case logs::TimePreset::LastHour:    chip = "last hour"; break;
                    case logs::TimePreset::Last24Hours: chip = "last 24 h"; break;
                    default:                            chip = "custom range"; break;
                }
            }
            if (levelFiltering) {
                if (!chip.empty()) {
                    chip += " · ";
                }
                const int enabled = static_cast<int>(m_showApp) + static_cast<int>(m_showDde) +
                                    static_cast<int>(m_showGui) + static_cast<int>(m_showService) +
                                    static_cast<int>(m_showOther);
                chip += std::format("levels {}/5", enabled);
            }
            if (searchActive) {
                if (!chip.empty()) {
                    chip += " · ";
                }
                chip += std::format("\"{}\"", std::string(search));
            }
            ImGui::TextDisabled("%s · %zu / %zu", chip.c_str(), visibleCount, entries.size());
        }

        ImGui::SameLine();
        ImGuiUtils::HelpMarker(
            "Selecting log lines:\n"
            "Click - select a line\n"
            "Ctrl+Click - add or remove a line\n"
            "Shift+Click - select a range\n"
            "Ctrl+C - copy the selection\n"
            "Right-click - copy menu\n\n"
            "Colors: green - completed actions, red - errors/timeouts.");

        if (ImGui::BeginPopup("##logs_filter_popup")) {
            const float labelW = ImGui::CalcTextSize("From").x +
                                 ImGui::GetStyle().ItemInnerSpacing.x * 2.0f;
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
            ImGui::TextUnformatted("Quick range");
            ImGui::SameLine();
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

            ImGui::Spacing();
            ImGui::TextUnformatted("Levels");
            ImGui::SameLine();
            ImGui::Checkbox("APP", &m_showApp);
            ImGui::SameLine();
            ImGui::Checkbox("DDE", &m_showDde);
            ImGui::SameLine();
            ImGui::Checkbox("GUI", &m_showGui);
            ImGui::SameLine();
            ImGui::Checkbox("Services", &m_showService);
            ImGui::SameLine();
            ImGui::Checkbox("Other", &m_showOther);

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
                m_showApp = m_showDde = m_showGui = m_showService = m_showOther = true;
                m_searchText.fill('\0');
                m_selected.clear();
                m_hasSelectionAnchor = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
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

        // Only the rows inside the scroll view are submitted: with tens of
        // thousands of entries the old loop built every row and its draw
        // commands on every frame, which is what dragged the app down.
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visibleIndices.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const size_t index = visibleIndices[static_cast<size_t>(row)];
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
                ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
                if (m_cache[index].tone == RowTone::Problem) {
                    textColor = ImGui::GetColorU32(sem.danger);
                } else if (m_cache[index].tone == RowTone::Success) {
                    textColor = ImGui::GetColorU32(sem.success);
                }
                drawList->PushClipRect(rowMin, rowMax, true);
                drawList->AddText(ImVec2(rowMin.x + ImGui::GetStyle().FramePadding.x, textY),
                                  textColor, entries[index].c_str());
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
                        copyToClipboard(joinEntries(entries, visible, nullptr), visibleCount);
                    }
                    if (ImGui::MenuItem("Clear selection", nullptr, false, !m_selected.empty())) {
                        clearSelection();
                    }
                    ImGui::Separator();
                    ImGui::TextDisabled("Click selects · Ctrl/Shift+Click extends · Ctrl+C copies");
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
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
                    m_showApp = m_showDde = m_showGui = m_showService = m_showOther = true;
                    m_searchText.fill('\0');
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
        if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_ENTRIES_CONFIRM_POPUP_NAME)) {
            ImGui::OpenPopup(DELETE_LOG_ENTRIES_CONFIRM_POPUP_NAME);
        }
        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(
            DELETE_LOG_ENTRIES_CONFIRM_POPUP_MIN_SIZE.x, DELETE_LOG_ENTRIES_CONFIRM_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(DELETE_LOG_ENTRIES_CONFIRM_POPUP_DEFAULT_SIZE);
        // &m_confirmDelete: the title-bar X closes the dialog like Cancel.
        if (ImGuiUtils::BeginPopupModalEx(DELETE_LOG_ENTRIES_CONFIRM_POPUP_NAME, &m_confirmDelete,
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
        } else if (m_confirmDelete && !ImGui::IsPopupOpen(DELETE_LOG_ENTRIES_CONFIRM_POPUP_NAME)) {
            // Closed through the app-wide close-popup shortcut (Esc) — drop the
            // pending request so it does not reopen on the next frame.
            m_confirmDelete = false;
        }
    }
}
