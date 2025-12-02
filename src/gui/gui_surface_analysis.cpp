#include "gui.h"

namespace gui {
    void GuiManager::calculateSurfaceProfile(int surfaceNumber, int sampling, double angle) {
        auto& targetSurface = [&]() -> const ZemaxDDE::SurfaceData& {
            auto target = zemaxDDEClient->getStorageTarget();
            assert(target == ZemaxDDE::StorageTarget::NOMINAL || target == ZemaxDDE::StorageTarget::TOLERANCED);
            switch (target) {
                case ZemaxDDE::StorageTarget::NOMINAL: return zemaxDDEClient->getNominalSurface();
                case ZemaxDDE::StorageTarget::TOLERANCED: return zemaxDDEClient->getTolerancedSurface();
                default: throw std::runtime_error("Unexpected storage target in calculateSurfaceProfile");
            }
        }();

        if (!targetSurface.isValid() || targetSurface.id != surfaceNumber) {
            logger.addLog("[GUI] No valid surface data for surface " + std::to_string(surfaceNumber));
            return;
        }

        double semiDiameter = targetSurface.semiDiameter;
        double step = targetSurface.diameter() / (sampling - 1);

        double rad = angle * M_PI / 180.0;
        double cosAngle = cos(rad);
        double sinAngle = sin(rad);

    #ifdef DEBUG_LOG
        logger.addLog("[GUI] Calculating profile for surface " + std::to_string(surfaceNumber) + 
                    " at angle " + std::to_string(angle) + "° with " + std::to_string(sampling) + " points");
    #endif

        for (int i = 0; i < sampling; ++i) {
            double r = -semiDiameter + i * step;
            double x = r * cosAngle;
            double y = r * sinAngle;
            
            zemaxDDEClient->getSag(surfaceNumber, x, y);
        }
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
            logger.addLog("[GUI] Failed to open file for writing: " + tempPath.string());
            return;
        }

        file << std::format("Listing of Surface Sag Cross Section Data\n\n");
        file << std::format("Surface {}.\n", surface.id);
        file << std::format("Coordinate units are {}.\n", getUnitString(surface.units, true));
        file << std::format("Units are {}.\n", getUnitString(surface.units, true));
        file << std::format("Width = {}, Decenter x = 0, y = 0 {}.\n", surface.diameter(), getUnitString(surface.units, true));
        file << "Cross section is oriented at an angle of 0 degrees.\n\n";

        file << std::format("{:<15}{:<15}{:<15}\n", "X-Coordinate", "Y-Coordinate", "Sag");

        for (const auto& point : surface.sagDataPoints) {
            file << std::format("{:<15.6e}{:<15.6e}{:<15.6e}\n", point.x, point.y, point.sag);
        }

        file.close();

        ShellExecuteW(nullptr, L"open", tempPath.c_str(), nullptr, nullptr, SW_SHOW);
        logger.addLog("[GUI] Sag profile saved to " + tempPath.string());
    }

    void GuiManager::renderProfileWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag) {
        if (!openFlag || !*openFlag) return;
        if (surface.sagDataPoints.empty()) return;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(title, openFlag)) {
            ImGui::End();
            return;
        }

        std::vector<double> x_vals, y_vals;

        for (const auto& [x, y, sag, alternateSag] : surface.sagDataPoints) {
            x_vals.push_back(x);
            y_vals.push_back(sag);
        }

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
            std::vector<double> x_nom, y_nom, x_tol, y_tol;

            for (const auto& p : nominal.sagDataPoints) {
                x_nom.push_back(p.x);
                y_nom.push_back(p.sag);
            }

            for (const auto& p : toleranced.sagDataPoints) {
                x_tol.push_back(p.x);
                y_tol.push_back(p.sag);
            }

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
        if (nominal.sagDataPoints.size() != toleranced.sagDataPoints.size()) return;

        if (ImGui::Begin("Sag Error (Toleranced - Nominal)", openFlag)) {
            std::vector<double> x, y;

            for (size_t i = 0; i < nominal.sagDataPoints.size(); ++i) {
                x.push_back(nominal.sagDataPoints[i].x);
                y.push_back(nominal.sagDataPoints[i].sag - toleranced.sagDataPoints[i].sag);
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
