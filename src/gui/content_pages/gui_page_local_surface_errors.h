#pragma once

struct LocalSurfaceErrorState {
    int tolerancedSurfaceIndex = 0;
    int nominalSurfaceIndex = 0;
    int sampling = 128;
    int numCrossSections = 1;
};

// Maximum number of sampling points along the diameter in Zemax (as per tool "Surface Sag Cross Section" from Zemax)
constexpr int MAX_SAMPLING = 16385;

// Minimum number of sampling points along the diameter in Zemax (Zemax requires at least 33 for valid profile calculation)
constexpr int MIN_SAMPLING = 33;
