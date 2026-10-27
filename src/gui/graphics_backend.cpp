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
#include "app/settings.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "assets/icons/fa/fa_compressed.h"
#include "app/config_path.h"
#include <cmath>
#include "logger/logger.h"

namespace gui {
    GraphicsBackend::~GraphicsBackend() {
        if (m_window) {
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

        // Register built-in themes
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

        // Merge Font Awesome 6 Regular (icons)
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

        // Geometry tokens are scaled by the DPI factor stored in ThemeManager.
        // Must be set before apply() so rounding/padding scale correctly even
        // before the first frame (ImGui::GetWindowDpiScale() is 0 until then).
        m_themeManager.setDpiScale(dpiScale);

        // Apply initial theme
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

    void GraphicsBackend::endFrame() {
        ImVec4 bg = m_themeManager.getClearColor();
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Multi-viewport: floating windows dragged beyond the main window get
        // rendered into their own GL contexts (ImGui examples contract). The
        // main context must be restored afterwards - the app loop swaps
        // buffers on m_window right after endFrame().
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            syncPlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(m_window);
        }
    }

    void GraphicsBackend::syncPlatformWindows() {
        // Win32-only polish on top of the GLFW multi-viewport support. The
        // GLFW backend has no notion of window ownership, so a floating panel
        // is an independent top-level window: it appears separately in the
        // task bar / Alt-Tab and, once hidden under other applications on
        // another monitor, can be hard to bring back.
        const HWND mainHwnd = glfwGetWin32Window(m_window);
        const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

        bool lastForegroundIsOurWindow = false;
        for (ImGuiViewport* viewport : platformIO.Viewports) {
            if (viewport->ID == ImGui::GetMainViewport()->ID) continue;
            auto* hwnd = static_cast<HWND>(viewport->PlatformHandleRaw);
            if (!hwnd) continue;
            if (hwnd == m_lastForegroundWindow) lastForegroundIsOurWindow = true;

            // Take ownership once: owned windows stay grouped with the main
            // one (taskbar/Alt-Tab activation, minimize/restore of the main
            // window treat the app as a single unit). Without it a secondary
            // window minimized behind an unrelated app is orphaned.
            const LONG_PTR ownedBy =
                ::GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT);
            if (ownedBy != reinterpret_cast<LONG_PTR>(mainHwnd)) {
                ::SetWindowLongPtrW(hwnd, GWLP_HWNDPARENT,
                    reinterpret_cast<LONG_PTR>(mainHwnd));
            }
        }

        const HWND foreground = ::GetForegroundWindow();
        // Raise-all trigger (variant "external focus only"): react when the
        // main window regains focus from another application. Switching focus
        // between this app's own windows keeps the user-arranged z-order of
        // the floating panels untouched. SWP_NOACTIVATE raises the windows
        // without stealing keyboard focus from the main window.
        if (foreground == mainHwnd && m_lastForegroundWindow &&
            m_lastForegroundWindow != mainHwnd && !lastForegroundIsOurWindow) {
            for (ImGuiViewport* viewport : platformIO.Viewports) {
                if (viewport->ID == ImGui::GetMainViewport()->ID) continue;
                auto* hwnd = static_cast<HWND>(viewport->PlatformHandleRaw);
                if (!hwnd) continue;
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
