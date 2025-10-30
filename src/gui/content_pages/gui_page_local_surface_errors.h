#pragma once

struct LocalSurfaceErrorState {
    int tolerancedSurfaceIndex = 0;
    int nominalSurfaceIndex = 0;
    int sampling = 128;
    int numCrossSections = 1;
};
