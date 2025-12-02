#pragma once

struct LocalSurfaceErrorsPageState {
    int tolerancedSurfaceIndex = 0;
    int nominalSurfaceIndex = 0;

    int tolerancedSampling = 128;
    double tolerancedAngle = 0.0;
    int nominalSampling = 128;
    double nominalAngle = 0.0;
};

// Maximum number of sampling points along the diameter in Zemax (as per tool "Surface Sag Cross Section" from Zemax)
constexpr int MAX_SAMPLING = 16385;

// Minimum number of sampling points along the diameter in Zemax (Zemax requires at least 33 for valid profile calculation)
constexpr int MIN_SAMPLING = 33;
