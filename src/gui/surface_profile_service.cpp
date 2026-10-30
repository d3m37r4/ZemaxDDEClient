#include <format>
#include <cmath>
#include <filesystem>
#include <cstring>
#include <exception>
#include <algorithm>
#include <cctype>

#include <windows.h>
#include <shellapi.h>
#include <nfd.h>

#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/imgui_test_engine/imgui_test_engine/thirdparty/stb/imstb_image_write.h"

#include "gui/surface_profile_service.h"
#include "gui/imgui_utils.h"
#include "gui/settings_manager.h"
#include "gui/utils.h"
#include "logger/logger.h"

namespace gui {
    namespace {
        ImPlotAxisFlags axisFlagsForGrid(bool showGrid) {
            return showGrid ? ImPlotAxisFlags_None : ImPlotAxisFlags_NoGridLines;
        }

        ImPlotSpec buildLineSpec(float lineWeight, float markerSize) {
            return ImPlotSpec(ImPlotProp_LineWeight, lineWeight,
                              ImPlotProp_MarkerSize, markerSize);
        }

        std::string surfaceSeriesCsv(const std::vector<double>& x, const std::vector<double>& y) {
            std::string csv;
            for (size_t i = 0; i < x.size(); ++i) {
                csv += std::to_string(x[i]);
                csv += ',';
                csv += std::to_string(y[i]);
                csv += '\n';
            }
            return csv;
        }

        std::string labelledSeriesCsv(const std::string& label, const std::vector<double>& x, const std::vector<double>& y) {
            std::string csv;
            for (size_t i = 0; i < x.size(); ++i) {
                csv += label;
                csv += ',';
                csv += std::to_string(x[i]);
                csv += ',';
                csv += std::to_string(y[i]);
                csv += '\n';
            }
            return csv;
        }

        // Default save-dialog name: "<kind>_surf<id>_<angle>deg[_slot<N>]".
        // Trailing zeros of the angle are trimmed (0.00 -> 0, 22.50 -> 22.5);
        // the slot suffix is dropped when the data has no DDE slot (-1).
        std::string plotImageName(const char* kind, const app::models::SurfaceData& surface) {
            std::string angle = std::format("{:.2f}", surface.angle);
            if (angle.find('.') != std::string::npos) {
                while (angle.back() == '0') angle.pop_back();
                if (angle.back() == '.') angle.pop_back();
            }
            std::string name = std::format("{}_surf{}_{}deg", kind, surface.id, angle);
            if (surface.sourceSlot >= 0) {
                name += std::format("_slot{}", surface.sourceSlot);
            }
            return name;
        }

        // Captures the plot frame (RGBA8, rows top-first) via the current
        // viewport's GL context; same contract as the test-engine capture in
        // imgui_test_engine/test_capture.cpp. ImPlot positions come in the
        // ImGui virtual-desktop space (a detached window's plot rect carries
        // its on-screen position), while glReadPixels addresses the client
        // area of the viewport's own OS window - hence the viewport->Pos
        // translation, scaled to physical pixels via FramebufferScale.
        bool capturePlotFrame(const ImVec2& frameMin, const ImVec2& frameSize,
                              std::vector<unsigned char>& pixels, std::string& error) {
            ImGuiViewport* viewport = ImGui::GetCurrentContext()->CurrentViewport;
            if (!viewport) {
                error = "no current ImGui viewport";
                return false;
            }
            GLFWwindow* ctxWindow = static_cast<GLFWwindow*>(viewport->PlatformHandle);
            if (!ctxWindow) {
                error = "viewport PlatformHandle is null";
                return false;
            }
            const float scaleX = viewport->FramebufferScale.x > 0.0f ? viewport->FramebufferScale.x : 1.0f;
            const float scaleY = viewport->FramebufferScale.y > 0.0f ? viewport->FramebufferScale.y : 1.0f;
            const int fbW = static_cast<int>(viewport->Size.x * scaleX);
            const int fbH = static_cast<int>(viewport->Size.y * scaleY);
            const int w = static_cast<int>(frameSize.x * scaleX);
            const int h = static_cast<int>(frameSize.y * scaleY);
            const int x = static_cast<int>((frameMin.x - viewport->Pos.x) * scaleX);
            const int glY = fbH - (static_cast<int>((frameMin.y - viewport->Pos.y) * scaleY) + h);
            if (w <= 0 || h <= 0 || x < 0 || glY < 0 ||
                x + w > fbW || glY + h > fbH) {
                error = std::format("capture region {}x{} at ({},{}) exceeds framebuffer {}x{}",
                                    w, h, x, glY, fbW, fbH);
                return false;
            }

            GLFWwindow* previous = glfwGetCurrentContext();
            const bool switched = (ctxWindow != previous);
            if (switched) {
                glfwMakeContextCurrent(ctxWindow);
            }
            pixels.assign(static_cast<size_t>(w) * h * 4, 0);
            glReadPixels(x, glY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            std::vector<unsigned char> lineTmp(static_cast<size_t>(w) * 4, 0);
            unsigned char* lineA = pixels.data();
            unsigned char* lineB = pixels.data() + static_cast<size_t>(w) * 4 * (h - 1);
            for (size_t i = 0; i < static_cast<size_t>(h) / 2; ++i) {
                std::memcpy(lineTmp.data(), lineA, static_cast<size_t>(w) * 4);
                std::memcpy(lineA, lineB, static_cast<size_t>(w) * 4);
                std::memcpy(lineB, lineTmp.data(), static_cast<size_t>(w) * 4);
                lineA += static_cast<size_t>(w) * 4;
                lineB -= static_cast<size_t>(w) * 4;
            }
            if (switched) {
                glfwMakeContextCurrent(previous);
            }
            return true;
        }

        const char* kDisplayUnitNames[] = { "mm", "μm" };

        int displayUnitCombo(const char* label, app::services::DisplayUnit& unit) {
            int staticUnit = static_cast<int>(unit);
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
            if (ImGui::Combo(label, &staticUnit, kDisplayUnitNames, IM_ARRAYSIZE(kDisplayUnitNames))) {
                unit = static_cast<app::services::DisplayUnit>(staticUnit);
                return 1;
            }
            return 0;
        }
    }

    void SurfaceProfileService::saveCrossSectionToFile(const app::models::SurfaceData& surface) {
        if (surface.sagDataPoints.empty()) {
            m_logger.addLog(std::format("[GUI] No Sag Cross Section data to save for surface {}", surface.id));
            return;
        }

        std::string content;
        content += "Listing of Surface Sag Cross Section Data\n\n";
        content += std::format("Surface {}.\n", surface.id);
        content += std::format("Coordinate units are {}.\n", gui::getUnitString(surface.units, true));
        content += std::format("Units are {}.\n", gui::getUnitString(surface.units, true));
        content += std::format("Width = {} {}.\n", surface.diameter(), gui::getUnitString(surface.units, true));
        content += std::format("Cross section is oriented at an angle of {:.2f} degrees.\n", surface.angle);
        content += std::format("Cross section calculated with {} sampling points.\n\n", surface.sampling);
        content += std::format("{:<15}{:<15}{:<15}\n", "X-Coordinate", "Y-Coordinate", "Sag");

        for (const auto& point : surface.sagDataPoints) {
            content += std::format("{:<15.6e}{:<15.6e}{:<15.6e}\n", point.x, point.y, point.sag);
        }

        auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_SagCrossSection_Temp.txt", content);
        if (!tempPathOpt) {
            m_logger.addLog("[GUI] Failed to create temporary file for Surface Sag Cross Section export");
            return;
        }

        ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
        m_logger.addLog(std::format("[GUI] Surface Sag Cross Section saved to {}", tempPathOpt->string()));
    }

    static constexpr const char* kColormapNames[] = { "Cool", "Aqua-Purple", "Ocean", "Aurora" };

    void SurfaceProfileService::renderToolbar(app::services::AxisUnits& units, bool is3D,
                                               int* colormapIdx, bool* showWorst, float* worstColor) {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu(ICON_FA_GEARS " Settings")) {
            ImGui::TextUnformatted("X axis");
            ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
            displayUnitCombo("##UnitX", units.x);

            if (is3D) {
                ImGui::TextUnformatted("Y axis");
                ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
                displayUnitCombo("##UnitY", units.y);
            }

            const char* yLabel = is3D ? "Z axis" : "Sag axis";
            ImGui::TextUnformatted(yLabel);
            ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
            displayUnitCombo(is3D ? "##UnitZ" : "##UnitY", is3D ? units.z : units.y);

            if (is3D && showWorst && worstColor) {
                ImGui::Separator();
                ImGui::Checkbox("##ShowWorst", showWorst);
                ImGui::SameLine();
                ImGui::TextUnformatted("Show worst");
                ImGui::SameLine();
                ImGui::ColorEdit3("##WorstColor", worstColor,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            }

            ImGui::EndMenu();
        }

        if (is3D && colormapIdx) {
            if (ImGui::BeginMenu(ICON_FA_PALETTE " Colormap")) {
                for (int i = 0; i < IM_ARRAYSIZE(kColormapNames); ++i) {
                    if (ImGui::MenuItem(kColormapNames[i], nullptr, *colormapIdx == i)) {
                        *colormapIdx = i;
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGuiUtils::ResetZoomMenuItem();
        ImGuiUtils::CopyDataMenuItem();
        ImGuiUtils::SaveImageMenuItem();

        ImGui::EndMenuBar();
    }

    bool SurfaceProfileService::savePlotImage(const ImVec2& frameMin, const ImVec2& frameSize, const std::string& defaultName) {
        try {
            std::vector<unsigned char> pixels;
            std::string error;
            if (!capturePlotFrame(frameMin, frameSize, pixels, error)) {
                m_logger.addLog(std::format("[GUI] Plot image capture failed: {}", error));
                return false;
            }

            static const nfdu8filteritem_t kFormats[] = {
                {"PNG image", "png"},
                {"JPEG image", "jpg/jpeg"},
                {"Windows Bitmap", "bmp"},
            };
            const nfdsavedialogu8args_t args = {
                kFormats, static_cast<nfdfiltersize_t>(IM_ARRAYSIZE(kFormats)),
                nullptr, defaultName.c_str(), {}, nullptr, nullptr, nullptr};
            nfdu8char_t* outPath = nullptr;
            const nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
            if (result == NFD_CANCEL) {
                return false;
            }
            if (result == NFD_ERROR || !outPath) {
                m_logger.addLog(std::format("[GUI] Save dialog failed: {}",
                                            NFD_GetError() ? NFD_GetError() : "unknown NFD error"));
                return false;
            }
            const std::string chosenPath = reinterpret_cast<const char*>(outPath);
            NFD_FreePathU8(outPath);

            // The dialog may return an extensionless path (file type set via
            // the filter dropdown); fall back to PNG and pick the encoder by
            // the final extension.
            std::string path = chosenPath;
            const size_t slashPos = path.find_last_of("/\\");
            if (path.find_last_of('.', slashPos + 1) == std::string::npos) {
                path += ".png";
            }
            std::string ext = path.substr(path.find_last_of('.') + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            const int w = static_cast<int>(frameSize.x);
            const int h = static_cast<int>(frameSize.y);
            const size_t rowBytes = pixels.size() / static_cast<size_t>(h);
            int written = 0;
            if (ext == "png") {
                written = stbi_write_png(path.c_str(), w, h, 4, pixels.data(),
                                         static_cast<int>(rowBytes));
            } else if (ext == "jpg" || ext == "jpeg") {
                written = stbi_write_jpg(path.c_str(), w, h, 4, pixels.data(), 90);
            } else if (ext == "bmp") {
                written = stbi_write_bmp(path.c_str(), w, h, 4, pixels.data());
            } else {
                m_logger.addLog(std::format("[GUI] Unsupported plot image format '.{}'", ext));
                return false;
            }
            if (written != 1) {
                m_logger.addLog(std::format("[GUI] Failed to write plot image to {}", path));
                return false;
            }
            m_logger.addLog(std::format("[GUI] Plot image saved to {}", path));
            return true;
        } catch (const std::exception& ex) {
            m_logger.addLog(std::format("[GUI] Plot image save threw: {}", ex.what()));
            return false;
        }
    }

    void SurfaceProfileService::renderSurfaceProfilePlot(const char* plotLabel, const app::models::SurfaceData& surface, const ImVec2& size) {
        if (surface.sagDataPoints.empty()) return;

        auto [x_vals, y_vals] = app::services::extractSagCoordinates(surface);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_scaled(x_vals.size()), y_scaled(y_vals.size());
        for (size_t i = 0; i < x_vals.size(); ++i) {
            x_scaled[i] = x_vals[i] * scaleX;
            y_scaled[i] = y_vals[i] * scaleY;
        }

        // The toolbar may issue a one-shot plot request (reset zoom / copy
        // data). Apply it only on the click frame, so it never refits or
        // re-copies away later zooms.
        const int plotRequest = ImGuiUtils::ConsumePlotRequest();
        if (plotRequest == ImGuiUtils::kPlotRequestReset) {
            ImPlot::SetNextAxesToFit();
        }
        if (plotRequest == ImGuiUtils::kPlotRequestCopy) {
            ImGui::SetClipboardText(surfaceSeriesCsv(x_scaled, y_scaled).c_str());
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("Sag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            if (plotRequest == ImGuiUtils::kPlotRequestSaveImage) {
                savePlotImage(ImPlot::GetPlotPos(), ImPlot::GetPlotSize(), plotImageName("sag", surface));
            }
            ImPlot::PlotLine("Surface", x_scaled.data(), y_scaled.data(), x_scaled.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileComparisonPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = app::services::extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = app::services::extractSagCoordinates(toleranced);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_nom_s(x_nom.size()), y_nom_s(y_nom.size());
        std::vector<double> x_tol_s(x_tol.size()), y_tol_s(y_tol.size());
        for (size_t i = 0; i < x_nom.size(); ++i) {
            x_nom_s[i] = x_nom[i] * scaleX;
            y_nom_s[i] = y_nom[i] * scaleY;
        }
        for (size_t i = 0; i < x_tol.size(); ++i) {
            x_tol_s[i] = x_tol[i] * scaleX;
            y_tol_s[i] = y_tol[i] * scaleY;
        }

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();
        if (plotRequest == ImGuiUtils::kPlotRequestReset) {
            ImPlot::SetNextAxesToFit();
        }
        if (plotRequest == ImGuiUtils::kPlotRequestCopy) {
            std::string csv = labelledSeriesCsv("Nominal", x_nom_s, y_nom_s)
                            + labelledSeriesCsv("Toleranced", x_tol_s, y_tol_s);
            ImGui::SetClipboardText(csv.c_str());
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("Sag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            if (plotRequest == ImGuiUtils::kPlotRequestSaveImage) {
                savePlotImage(ImPlot::GetPlotPos(), ImPlot::GetPlotSize(), plotImageName("compare", nominal));
            }
            ImPlot::PlotLine("Nominal", x_nom_s.data(), y_nom_s.data(), x_nom_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::PlotLine("Toleranced", x_tol_s.data(), y_tol_s.data(), x_tol_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileDeviationPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = app::services::extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = app::services::extractSagCoordinates(toleranced);

        if (x_nom.size() != x_tol.size()) return;

        std::vector<double> y_dev;
        y_dev.reserve(x_nom.size());
        for (size_t i = 0; i < x_nom.size(); ++i)
            y_dev.push_back(y_tol[i] - y_nom[i]);

        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_s(x_nom.size()), y_s(y_dev.size());
        for (size_t i = 0; i < x_nom.size(); ++i) {
            x_s[i] = x_nom[i] * scaleX;
            y_s[i] = y_dev[i] * scaleY;
        }

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();
        if (plotRequest == ImGuiUtils::kPlotRequestReset) {
            ImPlot::SetNextAxesToFit();
        }
        if (plotRequest == ImGuiUtils::kPlotRequestCopy) {
            ImGui::SetClipboardText(labelledSeriesCsv("Deviation", x_s, y_s).c_str());
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("ΔSag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            if (plotRequest == ImGuiUtils::kPlotRequestSaveImage) {
                savePlotImage(ImPlot::GetPlotPos(), ImPlot::GetPlotSize(), plotImageName("dev", nominal));
            }
            ImPlot::PlotLine("Deviation", x_s.data(), y_s.data(), x_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }
}
