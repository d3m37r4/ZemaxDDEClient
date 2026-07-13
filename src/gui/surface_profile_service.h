#pragma once

#include "app/services/surface_profile_service.h"
#include "lib/imgui/imgui.h"

class Logger;
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

        private:
            SettingsManager* m_settingsManager = nullptr;
    };
}
