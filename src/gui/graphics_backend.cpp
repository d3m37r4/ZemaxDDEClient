#include <stdexcept>
#include <filesystem>
#include <format>
#include <algorithm>

#include <windows.h>

#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/implot3d/implot3d.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "gui/graphics_backend.h"
#include "gui/constants.h"
#include "gui/plot_image_exporter.h"
#include "app/settings.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "assets/icons/fa/fa_compressed.h"
#include "app/config_path.h"
#include <cmath>
#include "logger/logger.h"

namespace gui {

    namespace {
        // Runtime-bound DWM attribute call (same pattern as
        // updateTitleBarDarkMode): dwmapi.dll is not linked directly, and
        // unsupported attributes (older systems) fail harmlessly.
        #ifndef DWMWA_WINDOW_CORNER_PREFERENCE
        #define DWMWA_WINDOW_CORNER_PREFERENCE 33
        #endif
        #ifndef DWMWCP_ROUNDSMALL
        #define DWMWCP_ROUNDSMALL 3
        #endif
        void dwmSetWindowAttribute(HWND hwnd, DWORD attribute,
            const void* value, DWORD size) {
            HMODULE dwmApi = ::LoadLibraryW(L"dwmapi.dll");
            if (!dwmApi) {
                return;
            }
            using DwmSetWindowAttributeFunc = HRESULT (WINAPI*)(
                HWND, DWORD, LPCVOID, DWORD);
            auto* dwmFunc = reinterpret_cast<DwmSetWindowAttributeFunc>(
                reinterpret_cast<void*>(::GetProcAddress(dwmApi,
                    "DwmSetWindowAttribute")));
            if (dwmFunc) {
                dwmFunc(hwnd, attribute, value, size);
            }
            ::FreeLibrary(dwmApi);
        }

        // Renderer_RenderWindow replacement for secondary viewports. The
        // vendored backend (imgui_impl_opengl3.cpp:1207) clears secondary
        // framebuffer with opaque black; a rounded window drawn there then
        // shows black corner slivers (and translucent layers composite on
        // black), while the main window composites on the theme canvas.
        // Clear with the same getClearColor() used for the main window so
        // the backdrop behind a floating panel matches the main window's
        // (imgui_impl_opengl3's public RenderDrawData renders the content).
        void renderViewportWindow(ImGuiViewport* viewport, void* userData) {
            if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear) && userData) {
                auto* self = static_cast<GraphicsBackend*>(userData);
                const ImVec4 clear = self->getClearColor();
                glClearColor(clear.x, clear.y, clear.z, clear.w);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            ImGui_ImplOpenGL3_RenderDrawData(viewport->DrawData);
        }
    } // namespace

    GraphicsBackend::~GraphicsBackend() {
        if (m_window) {
            // FBO/texture of the plot image exporter belong to this context.
            plot_export::shutdown();
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImPlot3D::DestroyContext();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
        }
    }

    void GraphicsBackend::initialize(GLFWwindow* window, Logger& logger, bool isLightTheme, float initialDpiScale) {
        m_window = window;
        m_logger = &logger;

        if (!m_window) {
            throw std::runtime_error("Invalid GLFW window");
        }

        m_themeManager.registerTheme(ThemeData::CreateThemeLight());
        m_themeManager.registerTheme(ThemeData::CreateThemeDark());

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImPlot3D::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        // Secondary viewports are plot regions, not standalone apps: drop
        // them from the task bar (the GLFW backend applies this by setting
        // WS_EX_TOOLWINDOW in Platform_ShowWindow) so Alt-Tab stays clean.
        io.ConfigViewportsNoTaskBarIcon = true;
        io.IniFilename = app::getImguiIniPath();

        float dpiScale = initialDpiScale;
        if (!std::isfinite(dpiScale) || dpiScale <= 0.0f) dpiScale = MIN_DPI_SCALE;
        dpiScale = std::clamp(dpiScale, MIN_DPI_SCALE, MAX_DPI_SCALE);

        // The GLFW backend recomputes io.DisplayFramebufferScale from the real
        // framebuffer/client ratio every NewFrame (imgui_impl_glfw.cpp
        // GetWindowSizeAndFramebufferScale), so seeding it manually is dead
        // code. Font sizes scale per-viewport through ConfigDpiScaleFonts
        // (imgui.cpp SetCurrentViewport -> style.FontScaleDpi); the manual
        // style.FontScaleDpi below is only the pre-first-frame seed.
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        // Load Segoe UI with Cyrillic support
        {
            std::filesystem::path fontPath = std::filesystem::path{L"C:\\Windows\\Fonts"} / L"segoeui.ttf";
            const std::string fontPathStr = fontPath.string();

            float baseFontSize = BASE_FONT_SIZE;
            ImFontConfig fontCfg;
            fontCfg.OversampleH = 2;
            fontCfg.OversampleV = 1;
            fontCfg.PixelSnapH = true;
            ImFont* font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize, &fontCfg, io.Fonts->GetGlyphRangesCyrillic());

            if (!font) {
                font = io.Fonts->AddFontFromFileTTF(fontPathStr.c_str(), baseFontSize);
            }

            if (!font) {
                logger.addLog("[GUI] Failed to load font 'segoeui.ttf'. Using default font.");
            }
        }

        // Merge mode: Font Awesome Regular is grafted onto the base font
        {
            ImFontConfig faCfg;
            faCfg.MergeMode = true;
            faCfg.PixelSnapH = true;
            faCfg.FontNo = 0;
            io.Fonts->AddFontFromMemoryCompressedTTF(
                fa_compressed_compressed_data, fa_compressed_compressed_size,
                BASE_FONT_SIZE * 0.8f, &faCfg);
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleDpi = dpiScale;

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ImGui_ImplOpenGL3_CreateDeviceObjects();

        // After Init: Init installs ImGui_ImplOpenGL3_RenderWindow (black
        // clear on secondary viewports), we replace it with the canvas
        // clear (renderViewportWindow). RenderPlatformWindowsDefault passes
        // the user data through below.
        ImGui::GetPlatformIO().Renderer_RenderWindow = &renderViewportWindow;

        // Geometry tokens are scaled by the DPI factor stored in ThemeManager.
        // Must be set before apply() so rounding/padding scale correctly even
        // before the first frame (ImGui::GetWindowDpiScale() is 0 until then).
        m_themeManager.setDpiScale(dpiScale);

        if (isLightTheme) {
            m_themeManager.apply(std::string{kThemeNameLight});
        } else {
            m_themeManager.apply(std::string{kThemeNameDark});
        }

        updateDpiStyle(dpiScale);

        logger.addLog(std::format("[GUI] GUI initialized (theme: {})", m_themeManager.currentThemeName()));
    }

    void GraphicsBackend::updateDpiStyle(float dpiScale) {
        dpiScale = std::clamp(dpiScale, MIN_DPI_SCALE, MAX_DPI_SCALE);

        // style.FontScaleDpi is intentionally NOT set here: with
        // io.ConfigDpiScaleFonts ImGui assigns it per-viewport on every
        // viewport switch (imgui.cpp SetCurrentViewport) and a manual write
        // would only be overwritten (or fight for) the next frame anyway.
        // This call rescales theme GEOMETRY tokens (rounding, padding,
        // spacing, border sizes), which ImGui never scales itself.
        m_themeManager.setDpiScale(dpiScale);
        m_themeManager.apply(m_themeManager.currentThemeName());
    }

    void GraphicsBackend::setFontScale(float scale) {
        if (!std::isfinite(scale)) scale = app::kDefaultFontScale;
        scale = std::clamp(scale, app::kMinFontScale, app::kMaxFontScale);
        ImGui::GetIO().FontGlobalScale = scale;
    }

    void GraphicsBackend::beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GraphicsBackend::endFrame(bool anyModalOpen) {
        ImVec4 bg = m_themeManager.getClearColor();
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Plot image export renders a captured draw list into an offscreen FBO:
        // the frame's draw lists are final only after Render(), and the main
        // context must still be current here.
        plot_export::renderPendingJob();

        // Multi-viewport: floating windows dragged beyond the main window get
        // rendered into their own GL contexts (ImGui examples contract). The
        // main context must be restored afterwards - the app loop swaps
        // buffers on m_window right after endFrame().
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            syncPlatformWindows(anyModalOpen);
            // nullptr here: the GLFW platform callbacks ignore it. `this`
            // flows into renderViewportWindow, which needs the theme's
            // clear color for the secondary viewport backdrops.
            ImGui::RenderPlatformWindowsDefault(nullptr, this);
            glfwMakeContextCurrent(m_window);
        }
    }

    void GraphicsBackend::syncPlatformWindows(bool anyModalOpen) {
        // Win32-only polish on top of the GLFW multi-viewport support. The
        // GLFW backend has no notion of window ownership, so a floating panel
        // is an independent top-level window: it appears separately in the
        // task bar / Alt-Tab and, once hidden under other applications on
        // another monitor, can be hard to bring back.
        const HWND mainHwnd = glfwGetWin32Window(m_window);
        const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

        std::vector<HWND> secondaryPanels;
        for (ImGuiViewport* viewport : platformIO.Viewports) {
            if (viewport->ID == ImGui::GetMainViewport()->ID) continue;
            auto* hwnd = static_cast<HWND>(viewport->PlatformHandleRaw);
            if (hwnd) secondaryPanels.push_back(hwnd);
        }

        // Band contract: our windows live directly around the main window and
        // never cross the z-order of unrelated applications. While a modal
        // dialog is open, a popup rendered into the main window's framebuffer
        // must cover the panels: the ownership is released (owned windows sit
        // above their owner unconditionally) and the panels sink just below
        // the main window. On close the ownership returns (the regular check
        // below) and the panels are inserted back just above the main window,
        // still beneath whatever foreign window sits above it.
        if (anyModalOpen && !m_modalOwnershipActive) {
            // Rising edge: capture our panels' current z-order (top to
            // bottom) so both directions restore the user's arrangement.
            m_modalOwnershipActive = true;
            m_modalPanelOrder.clear();
            for (HWND w = ::GetWindow(mainHwnd, GW_HWNDPREV); w;
                 w = ::GetWindow(w, GW_HWNDPREV)) {
                if (std::find(secondaryPanels.begin(), secondaryPanels.end(), w)
                    != secondaryPanels.end()) {
                    m_modalPanelOrder.push_back(w);
                }
            }
        }
        if (anyModalOpen) {
            // Sink the captured panels directly below the main window,
            // preserving their order. Idempotent: a window already in its
            // slot is skipped, so repeated calls do not reshuffle anything.
            HWND belowAnchor = mainHwnd;
            for (void* entry : m_modalPanelOrder) {
                auto* hwnd = static_cast<HWND>(entry);
                if (!::IsWindow(hwnd)) continue;
                if (::GetWindow(belowAnchor, GW_HWNDNEXT) != hwnd) {
                    ::SetWindowPos(hwnd, belowAnchor, 0, 0, 0, 0,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                }
                belowAnchor = hwnd;
            }
        } else if (m_modalOwnershipActive) {
            // Falling edge: place the panels just above the main window,
            // beneath whatever foreign window happens to sit above it.
            for (void* entry : m_modalPanelOrder) {
                auto* hwnd = static_cast<HWND>(entry);
                if (!::IsWindow(hwnd)) continue;
                ::SetWindowPos(hwnd, ::GetWindow(mainHwnd, GW_HWNDPREV),
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            }
            m_modalOwnershipActive = false;
            m_modalPanelOrder.clear();
        }

        bool lastForegroundIsOurWindow = false;
        for (HWND hwnd : secondaryPanels) {
            if (hwnd == m_lastForegroundWindow) lastForegroundIsOurWindow = true;

            // Taskbar/Alt-Tab hygiene for every secondary viewport: Dear ImGui
            // deliberately leaves modals out of the NoTaskBarIcon viewport flag,
            // so a modal that ends up in its own OS window would add a second
            // task-bar entry. A tool window never shows up there.
            const LONG_PTR exStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
            if (!(exStyle & WS_EX_TOOLWINDOW) || (exStyle & WS_EX_APPWINDOW)) {
                ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE,
                    (exStyle | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW);
            }
            // DWM rounding is otherwise applied only on ownership transitions;
            // a modal viewport may never transition (it is created unowned), so
            // prepare each window once on first sight.
            if (std::find(m_preparedViewports.begin(), m_preparedViewports.end(), hwnd) ==
                m_preparedViewports.end()) {
                m_preparedViewports.push_back(hwnd);
                const INT cornerPref = DWMWCP_ROUNDSMALL;
                dwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                    &cornerPref, sizeof(cornerPref));
            }

            const LONG_PTR ownedBy = ::GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT);
            const LONG_PTR wantedParent = anyModalOpen
                ? 0 : reinterpret_cast<LONG_PTR>(mainHwnd);
            if (ownedBy != wantedParent) {
                ::SetWindowLongPtrW(hwnd, GWLP_HWNDPARENT, wantedParent);
                // Win11: borderless (WS_POPUP) secondary windows get no DWM
                // corner rounding by default, so their silhouette reads as a
                // square while windows inside the main viewport show ImGui's
                // rounding. Opt in on ownership assignments (transitions
                // only, not per frame); the call is idempotent and older
                // hosts just reject the attribute. ROUNDSMALL (the OS's
                // menus/aux radius, 4 DIP) matches the theme design radius
                // DPI-scaled; the LARGE radius (8 DIP) would be twice as
                // round as every window we draw.
                const INT cornerPref = DWMWCP_ROUNDSMALL;
                dwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                    &cornerPref, sizeof(cornerPref));
            }
        }

        // Drop destroyed windows from the prepared list (handles are reused by
        // the OS, so a stale entry could skip the polish for a new window).
        std::erase_if(m_preparedViewports,
                      [](void* entry) { return !::IsWindow(static_cast<HWND>(entry)); });

        const HWND foreground = ::GetForegroundWindow();
        // Raise-all trigger (variant "external focus only"): react when the
        // main window regains focus from another application. Switching focus
        // between this app's own windows keeps the user-arranged z-order of
        // the floating panels untouched. SWP_NOACTIVATE raises the windows
        // without stealing keyboard focus from the main window. Skipped while
        // a modal is open: the panels must stay below the popup.
        if (foreground == mainHwnd && !anyModalOpen && m_lastForegroundWindow &&
            m_lastForegroundWindow != mainHwnd && !lastForegroundIsOurWindow) {
            for (HWND hwnd : secondaryPanels) {
                ::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            }
        }
        m_lastForegroundWindow = foreground;
    }

    void GraphicsBackend::updateTitleBarDarkMode(bool isDark) {
        if (!m_window) return;

        #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
        #define DWMWA_USE_IMMERSIVE_DARK_MODE 19
        #endif

        HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
        if (dwmApi) {
            using DwmSetWindowAttributeFunc = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            auto* dwmFunc = reinterpret_cast<DwmSetWindowAttributeFunc>(
                reinterpret_cast<void*>(GetProcAddress(dwmApi, "DwmSetWindowAttribute")));

            if (dwmFunc) {
                BOOL useDarkMode = isDark;
                HWND hwnd = glfwGetWin32Window(m_window);
                if (hwnd) {
                    dwmFunc(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
                }
            }
            FreeLibrary(dwmApi);
        }
    }
}
