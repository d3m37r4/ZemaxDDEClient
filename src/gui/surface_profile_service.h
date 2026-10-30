#pragma once

#include <optional>
#include <string>

#include "app/services/surface_profile_service.h"
#include "app/models/optical_system.h"
#include "lib/imgui/imgui.h"

namespace gui { class SettingsManager; }

namespace gui {

    class SurfaceProfileService : public app::services::SurfaceProfileService {
        public:
            using app::services::SurfaceProfileService::SurfaceProfileService;

            void setSettingsManager(SettingsManager* mgr) noexcept { m_settingsManager = mgr; }

            void saveCrossSectionToFile(const app::models::SurfaceData& surface);

            void renderSurfaceProfilePlot(const char* plotLabel, const app::models::SurfaceData& surface, const ImVec2& size);
            void renderProfileComparisonPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size);
            void renderProfileDeviationPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size);

            void renderToolbar(app::services::AxisUnits& units, bool is3D,
                               int* colormapIdx = nullptr, bool* showWorst = nullptr, float* worstColor = nullptr);

            app::models::OpticalSystemData m_frozenNominalOpticalSystem;
            app::models::OpticalSystemData m_frozenTolerancedOpticalSystem;

        private:
            /// Native save dialog; the format follows the chosen extension and
            /// the path falls back to .png when the dialog returns none.
            std::optional<std::string> choosePlotImagePath(const std::string& defaultName);
            /// Queues the current plot draw list for the offscreen export pass
            /// that runs in GraphicsBackend::endFrame().
            void requestPlotImageSave(const char* kind, const app::models::SurfaceData& surface);

            SettingsManager* m_settingsManager = nullptr;
    };
}
