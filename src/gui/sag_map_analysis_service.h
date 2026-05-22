#pragma once

#include <optional>
#include <vector>

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
            SagMapAnalysisService(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger);

            void calculateSurfaceMap(int surface, int numRadii, double angleStepDeg);
            // TODO: Re-enable Max-PV analysis
            // std::optional<MaxPVResult> findMaxPVSection() const;

            bool hasNominalReference() const { return m_ddeClient->getNominalSurface()->isValid(); }
            const ZemaxDDE::SurfaceData& getNominalReference() const { return *m_ddeClient->getNominalSurface(); }

            bool hasData() const { return !m_sections.empty(); }
            void clearData() { m_sections.clear(); }

            const std::vector<ZemaxDDE::SurfaceData>& getSections() const { return m_sections; }

            SagMapAnalysisState m_state;

        private:
            ZemaxDDE::ZemaxDDEClient* m_ddeClient;
            Logger& m_logger;

            std::vector<ZemaxDDE::SurfaceData> m_sections;
    };
}
