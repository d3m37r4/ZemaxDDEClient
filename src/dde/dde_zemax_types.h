#ifndef DDE_ZEMAX_TYPES_H
#define DDE_ZEMAX_TYPES_H

#include <string>

namespace ZemaxDDE {
    struct OpticalSystemData {
        std::string lensName;
        std::string fileName;
        int numSurfs = 0;
        int units = 0;
        int numFields = 0;
        int fieldType = 0;
        double xField[12] = {0};
        double yField[12] = {0};
        int numWaves = 0;
        int primWave = 0;
        double waveLen[24] = {0};
    };
}
#endif // DDE_ZEMAX_TYPES_H
