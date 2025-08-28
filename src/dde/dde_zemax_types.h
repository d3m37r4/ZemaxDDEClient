#ifndef DDE_ZEMAX_TYPES_H
#define DDE_ZEMAX_TYPES_H

#include <string>

namespace ZemaxDDE {
    constexpr int MIN_FIELDS = 1;
    constexpr int MAX_FIELDS = 12;

    constexpr int MIN_WAVES  = 1;
    constexpr int MAX_WAVES  = 24;

    constexpr int FIELD_ARRAY_SIZE = MAX_FIELDS + 1;
    constexpr int WAVE_ARRAY_SIZE  = MAX_WAVES + 1;

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
#endif // DDE_ZEMAX_TYPES_H
