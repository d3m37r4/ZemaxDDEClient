#include "gui.h"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <math.h>

namespace gui {
    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface) {
        std::vector<double> x_vals, y_vals;

        x_vals.reserve(surface.sagDataPoints.size());
        y_vals.reserve(surface.sagDataPoints.size());
        
        for (const auto& point : surface.sagDataPoints) {
            x_vals.push_back(point.x);
            y_vals.push_back(point.sag);
        }

        return {std::move(x_vals), std::move(y_vals)};
    }

    void GuiManager::calculateSurfaceProfile(int surfaceNumber, int sampling, double angle) {
        const auto targetStorage = zemaxDDEClient->getStorageTarget();
        assert(targetStorage == ZemaxDDE::StorageTarget::NOMINAL || targetStorage == ZemaxDDE::StorageTarget::TOLERANCED);
    
        const ZemaxDDE::SurfaceData& targetSurface = 
            (targetStorage == ZemaxDDE::StorageTarget::NOMINAL) 
                ? zemaxDDEClient->getNominalSurface() 
                : zemaxDDEClient->getTolerancedSurface();

        if (targetSurface.id != surfaceNumber || !targetSurface.isValid()) [[unlikely]] {
            logger.addLog(std::format("[GUI] No valid surface data for surface {}", surfaceNumber));
            return;
        }

        constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
        const double rad = angle * DEG_TO_RAD;
        const double cosAngle = cos(rad);
        const double sinAngle = sin(rad);
        double semiDiameter = targetSurface.semiDiameter;
        double step = targetSurface.diameter() / (sampling - 1);

    #ifdef DEBUG_LOG
        logger.addLog(std::format("[GUI] Calculating profile for surface {} at angle {}° with {} points", surfaceNumber, angle, sampling));
    #endif

        for (int i : std::views::iota(0, sampling)) {
            const double r = -semiDiameter + i * step;
            const double x = r * cosAngle;
            const double y = r * sinAngle;
            zemaxDDEClient->getSag(surfaceNumber, x, y);
        }     
        
        zemaxDDEClient->setSurfaceProfileMetadata(
            targetStorage,
            {.angle = angle, .sampling = sampling}
        );
    }

    void GuiManager::saveSagProfileToFile(const ZemaxDDE::SurfaceData& surface) {
        if (surface.sagDataPoints.empty()) {
            logger.addLog("[GUI] No sag profile data to save");
            return;
        }

        auto tempDir = std::filesystem::temp_directory_path();
        auto tempPath = tempDir / "ZemaxDDE_SagProfile.txt";

        std::ofstream file(tempPath);
        
        if (!file.is_open()) {
            logger.addLog(std::format("[GUI] Failed to open file for writing: {}", tempPath.string()));
            return;
        }

        file << std::format("Listing of Surface Sag Cross Section Data\n\n");
        file << std::format("Surface {}.\n", surface.id);
        file << std::format("Coordinate units are {}.\n", getUnitString(surface.units, true));
        file << std::format("Units are {}.\n", getUnitString(surface.units, true));
        file << std::format("Width = {}, Decenter x = 0, y = 0 {}.\n", surface.diameter(), getUnitString(surface.units, true));
        file << std::format("Cross section is oriented at an angle of {:.2f} degrees.\n", surface.angle);
        file << std::format("Cross section calculated with {} sampling points.\n\n", surface.sampling);

        file << std::format("{:<15}{:<15}{:<15}\n", "X-Coordinate", "Y-Coordinate", "Sag");

        for (const auto& point : surface.sagDataPoints) {
            file << std::format("{:<15.6e}{:<15.6e}{:<15.6e}\n", point.x, point.y, point.sag);
        }

        file.close();

        ShellExecuteW(nullptr, L"open", tempPath.c_str(), nullptr, nullptr, SW_SHOW);

        logger.addLog(std::format("[GUI] Sag profile saved to {}", tempPath.string()));
    }

    void GuiManager::renderProfileWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag) {
        if (!openFlag || !*openFlag) return;
        if (surface.sagDataPoints.empty()) return;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(title, openFlag)) {
            ImGui::End();
            return;
        }

        auto [x_vals, y_vals] = extractSagCoordinates(surface);

        if (ImPlot::BeginPlot(label, ImVec2(-1, -1))) {
            std::string unitName = getUnitString(surface.units);
            std::string xAxisLabel = std::format("X ({})", unitName);
            std::string yAxisLabel = std::format("Sag ({})", unitName);

            ImPlot::SetupAxes(xAxisLabel.c_str(), yAxisLabel.c_str());
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine(label, x_vals.data(), y_vals.data(), x_vals.size());
            ImPlot::EndPlot();
        }

        ImGui::End();
    }

    void GuiManager::renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag) {
        if (!openFlag || !*openFlag) return;

        if (ImGui::Begin("Profile Comparison", openFlag)) {
            auto [x_nom, y_nom] = extractSagCoordinates(nominal);
            auto [x_tol, y_tol] = extractSagCoordinates(toleranced);

            if (ImPlot::BeginPlot("##DetachedProfiles", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("X (mm)", "Sag (mm)");
                ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                ImPlot::PlotLine("Nominal", x_nom.data(), y_nom.data(), x_nom.size());
                ImPlot::PlotLine("Toleranced", x_tol.data(), y_tol.data(), x_tol.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
    }

    void GuiManager::renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag) {
        if (!openFlag || !*openFlag) return;

        auto [x_nom, y_nom] = extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = extractSagCoordinates(toleranced);

        if (x_nom.size() != x_tol.size()) return;

        if (ImGui::Begin("Sag Error (Toleranced - Nominal)", openFlag)) {
            std::vector<double> x, y;
            x.reserve(x_nom.size());
            y.reserve(x_nom.size());

            for (size_t i = 0; i < x_nom.size(); ++i) {
                x.push_back(x_nom[i]);
                y.push_back(y_nom[i] - y_tol[i]);
            }

            if (ImPlot::BeginPlot("##DetachedError", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("X (mm)", "ΔSag (mm)");
                ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
                ImPlot::PlotLine("Error", x.data(), y.data(), x.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
    }
}
