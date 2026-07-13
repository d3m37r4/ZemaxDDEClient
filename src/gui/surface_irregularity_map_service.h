#pragma once

#include "app/services/surface_map_service.h"
#include "lib/imgui/imgui.h"

class Logger;
namespace gui { class SettingsManager; }

namespace gui {

    class SurfaceIrregularityMapService : public app::services::SurfaceMapService {
        public:
            using app::services::SurfaceMapService::SurfaceMapService;

            void setSettingsManager(SettingsManager* mgr) noexcept { m_settingsManager = mgr; }

            void renderSurfacePlotLines(const ImVec2& size);
            void renderDeviationSurfacePlotLines(const ImVec2& size);

        private:
            SettingsManager* m_settingsManager = nullptr;
    };

    using MapWindowState = app::services::MapWindowState;
}
