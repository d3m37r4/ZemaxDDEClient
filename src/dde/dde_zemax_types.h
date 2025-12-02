#pragma once

#include <string>
#include <vector>
#include "dde_zemax_const.h"

namespace ZemaxDDE {
    struct Wavelength {
        double value = 0.0;
        double weight = 1.0;
    };

    struct SagData {
        double x = 0.0;
        double y = 0.0;
        double sag = 0.0;
        double alternateSag = 0.0;
    };

    struct SurfaceData {
        int id = -1;
        int units = 0;
        double semiDiameter = 0.0;
        std::string type = "Unknown";
        std::vector<SagData> sagDataPoints;
        bool isValid() const { return id >= 0; }
        double diameter() const { return 2.0 * semiDiameter; }

        void clear() {
            id = -1;
            units = 0;
            semiDiameter = 0.0;
            type = "Unknown";
            sagDataPoints.clear();
        }
    };

    struct OpticalSystemData {
        std::string lensName;
        std::string fileName;
        int numSurfs = 0;
        int units = 0;
        int stopSurf = 0;
        int nonAxialFlag = 0;
        int rayAimingType = 0;
        int adjustIndex = 0;
        double temp = 0.0;
        double pressure = 0.0;
        int globalRefSurf = 0;
        int numFields = 0;
        int fieldType = 0;
        double xField[FIELD_ARRAY_SIZE] = {0};
        double yField[FIELD_ARRAY_SIZE] = {0};
        double maxXField = 0.0;
        double maxYField = 0.0;
        int normalizationMethod = 0;
        int primWave = 0;
        int numWaves = 0;
        Wavelength waveData[WAVE_ARRAY_SIZE] = {};
    };
}
