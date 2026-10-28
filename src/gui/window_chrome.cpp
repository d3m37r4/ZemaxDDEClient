#include "gui/window_chrome.h"

// Internal API: ButtonBehavior on an explicit rect is the only way to place
// interactive widgets in the title bar (see titleBarButton comment below).
#include "imgui_internal.h"

#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "app/config_path.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace gui::chrome {

namespace {

// Shelf tiles are re-pushed every frame while their window stays minimized;
// entries a few frames stale belong to a window whose push site stopped
// running (data invalidated, panel hidden via the View menu) and are pruned
// instead of requiring explicit removal hooks.
constexpr unsigned kStaleFrameGap = 3;

// Layout of the "adaptive taskbar": nearly the whole window width is avail-
// able to the tiles (a small right margin keeps the strip clear of the
// status bar's right-side controls); they split it evenly, narrowing as new
// ones are added; once the minimum width is reached, tiles wrap to extra
// lines.
constexpr float SHELF_TILE_MIN_WIDTH = 90.0f;
constexpr float SHELF_TILE_MAX_WIDTH = 220.0f;
constexpr float SHELF_MAX_WIDTH_RATIO = 0.9f;
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

std::unordered_map<const void*, std::string>& persistentKeys() {
    static std::unordered_map<const void*, std::string> map;
    return map;
}

std::unordered_map<std::string, State>& pendingLoaded() {
    static std::unordered_map<std::string, State> map;
    return map;
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

/// Interactive square in the title bar, replicating the CloseButton pattern
/// (imgui_widgets.cpp:907). An item submitted here sits above the content
/// clip, so an InvisibleButton would be culled by ItemAdd -> IsClippedEx and
/// never react. ButtonBehavior on the explicit rect bypasses that and claims
/// HoveredId/ActiveId mid-frame, which keeps the end-of-frame window-move
/// logic (UpdateMouseMovingWindowEndFrame) from turning the click into a
/// title-bar drag. The ItemAdd result is ignored on purpose - like
/// close/collapse buttons, ours must react while clipped.
///
/// IMPORTANT: must be called inside a window-scope clip pushed by the caller
/// (PushClipRect over the whole window). IsMouseHoveringRect() - used by
/// ItemHoverable for the hit test - clips its rect against
/// CurrentWindow->ClipRect, which after Begin() is the content-only region
/// below the title bar (PushClipRect(InnerClipRect), imgui.cpp:8888); our
/// button rects sit above it and would never pass. ImGui's own title-bar
/// buttons run inside Begin() BEFORE that clip is applied - we recreate
/// their environment here.
bool titleBarButton(const ImVec2& pos, const char* str_id, float size) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));

    (void)ImGui::ItemAdd(bb, id);
    bool hovered = false;
    bool held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held,
        ImGuiButtonFlags_None);
    if (hovered || held) {
        window->DrawList->AddRectFilled(bb.Min, bb.Max,
            ImGui::GetColorU32(held ? ImGuiCol_ButtonActive
                                    : ImGuiCol_ButtonHovered));
    }
    return pressed;
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

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImRect title = window->TitleBarRect();
    const float btnSz = ImGui::GetFontSize();
    const ImU32 glyphColor = ImGui::GetColorU32(ImGuiCol_Text);
    const float extent = btnSz * 0.5f * 0.7f;

    // Same layout math as RenderWindowTitleBarContents (imgui.cpp:7865): the
    // close square is FontSize wide, anchored FramePadding.x from the window
    // right edge; our buttons reuse its FontSize + ItemInnerSpacing pitch
    // leftwards, maximize nearest to the close button (Windows order x-□-−).
    const ImVec2 closePos(title.Max.x - style.FramePadding.x - btnSz,
        title.Min.y + style.FramePadding.y);
    const float pitch = btnSz + style.ItemInnerSpacing.x;
    const ImVec2 maximizePos(closePos.x - pitch, closePos.y);
    const ImVec2 minimizePos(closePos.x - 2.0f * pitch, closePos.y);
    const ImVec2 maximizeCenter(maximizePos.x + btnSz * 0.5f,
        maximizePos.y + btnSz * 0.5f);
    const ImVec2 minimizeCenter(minimizePos.x + btnSz * 0.5f,
        minimizePos.y + btnSz * 0.5f);

    // Everything below (buttons + glyphs) lives in the title bar, outside
    // the window ClipRect the content gets after Begin() — and worse,
    // IsMouseHoveringRect() shrinks its hit test by exactly that ClipRect,
    // which is why the buttons would stay dead and the click would fall
    // through into a title-bar drag. Push a full-window clip to recreate the
    // environment ImGui's own close button has inside Begin(), pop it before
    // the content is rendered.
    const ImVec2 winPos = window->Pos;
    const ImVec2 winMax(winPos.x + window->SizeFull.x,
        winPos.y + window->SizeFull.y);
    ImGui::PushClipRect(winPos, winMax, false);

    if (titleBarButton(maximizePos, "##Maximize", btnSz)) {
        if (!st.maximized) {
            st.restorePos = window->Pos;
            st.restoreSize = window->Size;
            st.maximized = true;
        } else {
            st.maximized = false;
            st.wantRestoreRect = true;
        }
    }
    if (titleBarButton(minimizePos, "##Minimize", btnSz)) {
        // Preserve the maximized flag: restore brings the window back into
        // the state it was minimized from (floating rect covers the rest).
        if (!st.maximized) {
            st.restorePos = window->Pos;
            st.restoreSize = window->Size;
        }
        st.minimized = true;
    }

    // Glyphs live in the title bar, outside the content clip the window draw
    // list has after Begin(); the full-window clip pushed above also covers
    // them.
    ImDrawList* dl = window->DrawList;
    drawWindowRects(dl, maximizeCenter, extent, glyphColor, st.maximized);
    drawMinimizeGlyph(dl, minimizeCenter, extent, glyphColor);
    ImGui::PopClipRect();
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
    // Self-sufficient strip height: computeStatusBarHeight() no longer
    // includes the shelf (it must stay constant for dock-area layout), so the
    // overlay refreshes pruning and its cached height here.
    currentShelfHeight();
    auto& items = shelfItems();
    if (items.empty() || storedShelfHeight() <= 0.0f) {
        return;
    }
    // The strip is a transparent overlay hovering over the bottom edge of
    // floating windows, right above the status bar: it never reserves layout
    // height, so windows under it keep their size (user requirement).
    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 winPos(vp->Pos.x,
        vp->Pos.y + vp->Size.y - storedBottomBlockHeight()
            - storedShelfHeight());
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
    // Render draw order within a viewport is g.Windows display order
    // (imgui.cpp:6562), NOT submission order: any window the user clicks
    // gets BringWindowToDisplayFront (imgui.cpp:13925) and starts drawing
    // on top of this strip, burying the tiles under overlapping floating
    // window bottoms. Re-assert front placement every frame; yield to an
    // open modal so dialogs stay above the strip.
    if (ImGui::GetTopMostAndVisiblePopupModal() == nullptr) {
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    }

    const float gap = ImGuiUtils::DpiScale(SHELF_TILE_GAP);
    const float rowGap = ImGuiUtils::DpiScale(SHELF_ROW_GAP);
    const float tileH = ImGui::GetFrameHeight();
    const float inset = ImGuiUtils::DpiScale(SHELF_TILE_TEXT_INSET);
    const ShelfLayout l = shelfLayout(int(items.size()));

    // Rows stack from the status bar upward: the bottom row (adjacent to
    // the bar) is filled first with the first-minimized tiles, overflow
    // rows go above, so the last (partial) row always ends up on top and
    // the strip visually grows away from the bar.
    ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();
    const float rowStep = tileH + rowGap;

    int index = 0;
    for (auto& item : items) {
        const int rowFromBottom = index / l.perRow;
        const float x = gap + float(index % l.perRow) * (l.tileWidth + gap);
        const float y = rowGap
            + float(l.rows - 1 - rowFromBottom) * rowStep;
        const ImVec2 tileMin(x, y);
        const ImVec2 tileMax(x + l.tileWidth, y + tileH);
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
        ++index;
    }

    ImGui::End();
}

// --- Session persistence (chrome_state.json) ---------------------------- //

nlohmann::json stateToJson(const State& st) {
    return {
        {"minimized", st.minimized},
        {"restorePos", {st.restorePos.x, st.restorePos.y}},
        {"restoreSize", {st.restoreSize.x, st.restoreSize.y}},
    };
}

State stateFromJson(const nlohmann::json& j) {
    State st;
    st.minimized = j.value("minimized", false);
    if (j.contains("restorePos") && j["restorePos"].is_array()
        && j["restorePos"].size() == 2) {
        st.restorePos.x = j["restorePos"][0].get<float>();
        st.restorePos.y = j["restorePos"][1].get<float>();
    }
    if (j.contains("restoreSize") && j["restoreSize"].is_array()
        && j["restoreSize"].size() == 2) {
        st.restoreSize.x = j["restoreSize"][0].get<float>();
        st.restoreSize.y = j["restoreSize"][1].get<float>();
    }
    return st;
}

void setPersistentKey(const void* token, std::string_view key) {
    if (key.empty()) {
        return;
    }
    auto& keyFor = persistentKeys();
    auto it = keyFor.find(token);
    if (it != keyFor.end()) {
        if (it->second == key) {
            return; // Same key, same frame: reassociation is a no-op.
        }
        return; // Already bound once; drifting keys would desync the file.
    }
    keyFor.emplace(token, std::string(key));

    const std::string str(key);
    auto& pending = pendingLoaded();
    auto loaded = pending.find(str);
    if (loaded == pending.end()) {
        return;
    }
    State& st = state(token);
    st.minimized = loaded->second.minimized;
    st.restorePos = loaded->second.restorePos;
    st.restoreSize = loaded->second.restoreSize;
    pending.erase(loaded);
}

void loadState() {
    std::ifstream in(app::getChromeStatePath());
    if (!in) {
        return;
    }
    try {
        nlohmann::json j;
        in >> j;
        if (!j.is_object()) {
            return;
        }
        auto& pending = pendingLoaded();
        for (auto& el : j.items()) {
            pending[el.key()] = stateFromJson(el.value());
        }
    } catch (const std::exception&) {
        // Corrupt or foreign file: default to session-only chrome state
        // instead of failing startup.
    }
}

void saveState() {
    const auto& keyFor = persistentKeys();
    if (keyFor.empty()) {
        return;
    }
    nlohmann::json j;
    const auto& reg = registry();
    for (const auto& [token, key] : keyFor) {
        auto st = reg.find(token);
        if (st != reg.end()) {
            j[key] = stateToJson(st->second);
        }
    }
    std::ofstream out(app::getChromeStatePath());
    out << j.dump(4);
}

} // namespace gui::chrome
