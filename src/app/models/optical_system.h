#pragma once

#include <array>
#include <string>

#include "dde/constants.h"

namespace app::models {

    struct Wavelength {
        double value = 0.0;
        double weight = 1.0;
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
        std::array<double, ZemaxDDE::FIELD_ARRAY_SIZE> xField{};
        std::array<double, ZemaxDDE::FIELD_ARRAY_SIZE> yField{};
        double maxXField = 0.0;
        double maxYField = 0.0;
        int normalizationMethod = 0;
        int primWave = 0;
        int numWaves = 0;
        std::array<Wavelength, ZemaxDDE::WAVE_ARRAY_SIZE> waveData{};
    };

}
