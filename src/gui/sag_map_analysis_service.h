#pragma once

#include <optional>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"

class Logger;

namespace gui {
    struct SagMapAnalysisState {
        int nominalSurfaceIndex = 0;
        int nominalSampling = 128;
        double nominalAngle = 0.0;

        int tolerancedSurfaceIndex = 0;
        int tolerancedSampling = 128;
        double tolerancedAngleStep = 1.0;
    };

    struct MaxPVResult {
        double angle;
        double peak;
        double valley;
        double pv;
    };

    class SagMapAnalysisService {
        public:
            SagMapAnalysisService(DDEConnectionManager* connectionManager, Logger& logger);

            void calculateSurfaceMap(int surface, int numRadii, double angleStepDeg);

            bool hasNominalReference() const;
            const ZemaxDDE::SurfaceData& getNominalReference() const;

            bool hasData() const { return !m_sections.empty(); }
            void clearData() { m_sections.clear(); }

            const std::vector<ZemaxDDE::SurfaceData>& getSections() const { return m_sections; }

            SagMapAnalysisState m_state;

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const { return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr; }

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;

            std::vector<ZemaxDDE::SurfaceData> m_sections;
    };
}
