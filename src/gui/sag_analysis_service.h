#pragma once

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

#include "dde/client.h"
#include "gui/content_pages/page_surface_sag_analysis.h"

class Logger;

namespace gui {
    // Free utility function — extracts X and Sag coordinates from surface data
    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface);

    // Write sag cross section data to a temporary text file
    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

    /**
     * @brief Sag analysis service — business logic for surface sag cross section analysis.
     *        Handles data calculation, export, and ImPlot rendering.
     */
    class SagAnalysisService {
        public:
            SagAnalysisService(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);

            // Business logic
            void calculateSagCrossSection(int surface, int sampling, double angle = 0.0);
            void saveCrossSectionToFile(const ZemaxDDE::SurfaceData& surface);

            // Detached plot windows
            void renderCrossSectionWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag);
            void renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);
            void renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);

            // Window visibility state
            bool m_showTolerancedSagWindow{false};
            bool m_showNominalSagWindow{false};
            bool m_showComparisonWindow{false};
            bool m_showErrorWindow{false};

            // Page state
            SurfaceSagAnalysisPageState m_state;

        private:
            ZemaxDDE::ZemaxDDEClient* m_ddeClient;
            Logger& m_logger;
    };
}
