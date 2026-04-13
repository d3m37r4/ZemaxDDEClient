#pragma once

struct SurfaceSagAnalysisPageState {
    int tolerancedSurfaceIndex = 0;
    int nominalSurfaceIndex = 0;

    int tolerancedSampling = 128;
    double tolerancedAngle = 0.0;
    int nominalSampling = 128;
    double nominalAngle = 0.0;
};
