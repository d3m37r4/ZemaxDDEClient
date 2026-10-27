#include "gui/window_chrome.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace gui::chrome {

namespace {

// Shelf tiles are re-pushed every frame while their window stays minimized;
// entries a few frames stale belong to a window whose push site stopped
// running (data invalidated, panel hidden via the View menu) and are pruned
// instead of requiring explicit removal hooks.
constexpr unsigned kStaleFrameGap = 3;

// Layout of the "adaptive taskbar": a fixed share of the dock-area width is
// split evenly between the existing tiles, narrowing them as new ones are
// added; once the minimum width is reached, tiles wrap to extra lines.
constexpr float SHELF_TILE_MIN_WIDTH = 90.0f;
constexpr float SHELF_TILE_MAX_WIDTH = 220.0f;
constexpr float SHELF_MAX_WIDTH_RATIO = 0.5f;
constexpr float SHELF_TILE_GAP = 4.0f;
constexpr float SHELF_ROW_GAP = 2.0f;
constexpr float SHELF_TILE_TEXT_INSET = 8.0f;

struct ShelfItem {
    const void* token = nullptr;
    std::string title;
    unsigned lastSeenFrame = 0;
};

std::unordered_map<const void*, State>& registry() {
    static std::unordered_map<const void*, State> map;
    return map;
}

std::vector<ShelfItem>& shelfItems() {
    static std::vector<ShelfItem> items;
    return items;
}

float& storedShelfHeight() {
    static float height = 0.0f;
    return height;
}

float& storedBottomBlockHeight() {
    static float height = 0.0f;
    return height;
}

State& state(const void* token) {
    return registry()[token];
}

void pruneShelf() {
    const unsigned now = ImGui::GetFrameCount();
    auto& items = shelfItems();
    items.erase(std::remove_if(items.begin(), items.end(),
                    [now](const ShelfItem& i) {
                        return (now - i.lastSeenFrame) > kStaleFrameGap;
                    }),
        items.end());
}

struct ShelfLayout {
    float tileWidth = 0.0f;
    int perRow = 1;
    int rows = 1;
};

ShelfLayout shelfLayout(int count) {
    const float minW = ImGuiUtils::DpiScale(SHELF_TILE_MIN_WIDTH);
    const float maxW = ImGuiUtils::DpiScale(SHELF_TILE_MAX_WIDTH);
    const float availW = ImGui::GetMainViewport()->Size.x * SHELF_MAX_WIDTH_RATIO;
    ShelfLayout l;
    const int perRowCap = std::max(1, int(availW / minW));
    if (count <= perRowCap) {
        // All tiles fit one row: each added tile narrows all of them (down
        // to the shared even share); few tiles stay capped at the max width.
        l.perRow = count;
        l.tileWidth = std::clamp(availW / float(count), minW, maxW);
    } else {
        l.perRow = perRowCap;
        l.tileWidth = minW;
    }
    l.rows = (count + l.perRow - 1) / l.perRow;
    return l;
}

void drawMinimizeGlyph(ImDrawList* dl, const ImVec2& center, float extent, ImU32 color) {
    dl->AddLine(ImVec2(center.x - extent, center.y),
        ImVec2(center.x + extent, center.y), color, ImGuiUtils::DpiScale(1.4f));
}

/// Overlapping square outlines, the Windows "restore" glyph: the back
/// square shifted up-right behind the front one.
void drawWindowRects(ImDrawList* dl, const ImVec2& center, float extent, ImU32 color,
                     bool withBackSquare) {
    const ImVec2 half(extent * 0.85f, extent * 0.65f);
    const float thick = ImGuiUtils::DpiScale(1.4f);
    if (withBackSquare) {
        const ImVec2 shift(ImGuiUtils::DpiScale(2.5f), -ImGuiUtils::DpiScale(2.5f));
        dl->AddRect(ImVec2(center.x + shift.x - half.x, center.y + shift.y - half.y),
            ImVec2(center.x + shift.x + half.x, center.y + shift.y + half.y),
            color, 0.0f, 0, ImGuiUtils::DpiScale(1.2f));
    }
    dl->AddRect(ImVec2(center.x - half.x, center.y - half.y),
        ImVec2(center.x + half.x, center.y + half.y), color, 0.0f, 0, thick);
}

/// Hit-testable square anchored at the title bar; the content cursor is
/// restored afterwards so the window body keeps its own coordinates.
bool titleBarButton(const ImVec2& rectMin, const char* id) {
    const float size = ImGui::GetFrameHeight();
    const ImVec2 rectMax(rectMin.x + size, rectMin.y + size);
    const ImVec2 cursorBackup = ImGui::GetCursorScreenPos();

    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(rectMin);
    ImGui::InvisibleButton(id, ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    ImGui::PopID();
    ImGui::SetCursorScreenPos(cursorBackup);

    if (hovered || active) {
        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax,
            ImGui::GetColorU32(active ? ImGuiCol_ButtonActive
                                      : ImGuiCol_ButtonHovered));
    }
    return clicked;
}

/// Cuts the label to the available pixel width with an ellipsis; assumes
/// @p label already ends with "..." when called.
std::string truncateToWidth(ImFont* font, float fontSize, float avail,
    const std::string& title) {
    if (font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, title.c_str()).x <= avail) {
        return title;
    }
    std::string label = title + "...";
    while (label.size() > 4
        && font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label.c_str()).x > avail) {
        // Drop one character before the ellipsis tail.
        label.erase(label.size() - 4, 1);
    }
    return label;
}

} // namespace

bool minimizedAndShelved(const void* token, std::string_view title) {
    State& st = state(token);
    if (!st.minimized) {
        return false;
    }
    auto& items = shelfItems();
    for (auto& i : items) {
        if (i.token == token) {
            i.title.assign(title);
            i.lastSeenFrame = ImGui::GetFrameCount();
            return true;
        }
    }
    items.push_back({token, std::string(title), unsigned(ImGui::GetFrameCount())});
    return true;
}

void applyFrameRect(const void* token) {
    State& st = state(token);
    if (st.maximized) {
        // Dock area: below the menu bar, above the status bar + shelf. The
        // bottom block height is reported by the status bar every frame.
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float menuBar = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(
            ImVec2(vp->Pos.x, vp->Pos.y + menuBar), ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(vp->Size.x, vp->Size.y - menuBar - storedBottomBlockHeight()),
            ImGuiCond_Always);
        return;
    }
    if (st.wantRestoreRect) {
        ImGui::SetNextWindowPos(st.restorePos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(st.restoreSize, ImGuiCond_Always);
        st.wantRestoreRect = false;
    }
}

bool isMaximized(const void* token) {
    return state(token).maximized;
}

void setBottomBlockHeight(float height) {
    storedBottomBlockHeight() = height;
}

void drawTitleBarButtons(const void* token, bool* open) {
    // The open flag belongs to the caller: minimize never closes the window,
    // it only hides it behind the shelf tile.
    (void)open;
    State& st = state(token);
    if (ImGui::IsWindowDocked()) {
        // The dock node owns the geometry and its tab owns the close button;
        // a floating-period state must not reassert itself on the next undock.
        if (st.maximized || st.minimized || st.wantRestoreRect) {
            st.maximized = false;
            st.minimized = false;
            st.wantRestoreRect = false;
        }
        return;
    }

    const float size = ImGui::GetFrameHeight();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const float winW = ImGui::GetWindowSize().x;
    const ImU32 glyphColor = ImGui::GetColorU32(ImGuiCol_Text);
    const float extent = size * 0.5f * 0.7f;

    // The x close button occupies the rightmost title-height square; our
    // buttons stack leftwards from it, maximize first (Windows order).
    const float maximizeX = winPos.x + winW - 2.0f * size;
    const float minimizeX = winPos.x + winW - 3.0f * size;
    const ImVec2 maximizeCenter(maximizeX + size * 0.5f, winPos.y + size * 0.5f);
    const ImVec2 minimizeCenter(minimizeX + size * 0.5f, winPos.y + size * 0.5f);

    if (titleBarButton(ImVec2(maximizeX, winPos.y), "##Maximize")) {
        if (!st.maximized) {
            st.restorePos = winPos;
            st.restoreSize = ImGui::GetWindowSize();
            st.maximized = true;
        } else {
            st.maximized = false;
            st.wantRestoreRect = true;
        }
    }
    if (titleBarButton(ImVec2(minimizeX, winPos.y), "##Minimize")) {
        // Preserve the maximized flag: restore brings the window back into
        // the state it was minimized from (floating rect covers the rest).
        if (!st.maximized) {
            st.restorePos = winPos;
            st.restoreSize = ImGui::GetWindowSize();
        }
        st.minimized = true;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    drawWindowRects(dl, maximizeCenter, extent, glyphColor, st.maximized);
    drawMinimizeGlyph(dl, minimizeCenter, extent, glyphColor);
}

float currentShelfHeight() {
    pruneShelf();
    auto& items = shelfItems();
    if (items.empty()) {
        storedShelfHeight() = 0.0f;
        return 0.0f;
    }
    const ShelfLayout l = shelfLayout(int(items.size()));
    const float tileH = ImGui::GetFrameHeight();
    const float rowGap = ImGuiUtils::DpiScale(SHELF_ROW_GAP);
    // One gap above and below the tiles keeps the strip breathing.
    const float h = float(l.rows) * tileH + float(l.rows + 1) * rowGap;
    storedShelfHeight() = h;
    return h;
}

void renderShelf() {
    auto& items = shelfItems();
    if (items.empty() || storedShelfHeight() <= 0.0f) {
        return;
    }
    // The strip is invisible on purpose: no frame, no separator, only the
    // tile buttons themselves. Sits right above the status bar.
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float posBottomBlock = storedBottomBlockHeight();
    const ImVec2 winPos(vp->Pos.x, vp->Pos.y + vp->Size.y - posBottomBlock);
    ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(vp->Size.x, storedShelfHeight()), ImGuiCond_Always);
    ImGui::SetNextWindowViewport(vp->ID);

    if (!ImGui::Begin("##WindowShelf", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus)) {
        ImGui::End();
        return;
    }

    const float gap = ImGuiUtils::DpiScale(SHELF_TILE_GAP);
    const float rowGap = ImGuiUtils::DpiScale(SHELF_ROW_GAP);
    const float tileH = ImGui::GetFrameHeight();
    const float inset = ImGuiUtils::DpiScale(SHELF_TILE_TEXT_INSET);
    const ShelfLayout l = shelfLayout(int(items.size()));

    float cursorY = rowGap;
    int indexInRow = 0;
    ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();

    for (auto& item : items) {
        const float x = gap + float(indexInRow) * (l.tileWidth + gap);
        const ImVec2 tileMin(x, cursorY);
        const ImVec2 tileMax(x + l.tileWidth, cursorY + tileH);
        const ImVec2 absMin(winPos.x + tileMin.x, winPos.y + tileMin.y);

        ImGui::PushID(item.token);
        ImGui::SetCursorScreenPos(
            ImVec2(winPos.x + tileMin.x, winPos.y + tileMin.y));
        ImGui::InvisibleButton("##tile", ImVec2(l.tileWidth, tileH));
        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(absMin,
            ImVec2(absMin.x + l.tileWidth, absMin.y + tileH),
            ImGui::GetColorU32(active ? ImGuiCol_ButtonActive
                                      : hovered ? ImGuiCol_ButtonHovered
                                                : ImGuiCol_Button));

        // Truncate the label to the tile width; hover shows the full title.
        const std::string label = truncateToWidth(font, fontSize,
            l.tileWidth - 2.0f * inset, item.title);
        const ImVec2 textSize =
            font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label.c_str());
        dl->AddText(ImVec2(absMin.x + inset, absMin.y + (tileH - textSize.y) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

        if (hovered) {
            ImGui::SetTooltip("%s", item.title.c_str());
        }
        if (clicked) {
            State& st = state(item.token);
            st.minimized = false;
            st.wantRestoreRect = true;
        }

        ++indexInRow;
        if (indexInRow >= l.perRow) {
            indexInRow = 0;
            cursorY += tileH + rowGap;
        }
    }

    ImGui::End();
}

} // namespace gui::chrome
