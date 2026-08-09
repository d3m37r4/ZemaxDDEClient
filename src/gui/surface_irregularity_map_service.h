#pragma once

#include "app/services/surface_map_service.h"
#include "app/models/optical_system.h"
#include "lib/imgui/imgui.h"

namespace gui { class SettingsManager; }

namespace gui {

    class SurfaceIrregularityMapService : public app::services::SurfaceMapService {
        public:
            using app::services::SurfaceMapService::SurfaceMapService;

            void setSettingsManager(SettingsManager* mgr) noexcept { m_settingsManager = mgr; }

            void renderSurfacePlotLines(const ImVec2& size);
            void renderDeviationSurfacePlotLines(const ImVec2& size);

            app::models::OpticalSystemData m_frozenNominalOpticalSystem;
            app::models::OpticalSystemData m_frozenMapOpticalSystem;

        private:
            SettingsManager* m_settingsManager = nullptr;
    };

    using MapWindowState = app::services::MapWindowState;
}
