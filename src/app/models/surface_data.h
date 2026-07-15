#pragma once

#include <string>
#include <vector>

namespace app::models {

    struct SagData {
        double x = 0.0;
        double y = 0.0;
        double sag = 0.0;
        double alternateSag = 0.0;
    };

    struct SurfaceData {
        int id = -1;
        int units = 0;
        int sampling = 0;
        double angle = 0.0;
        double semiDiameter = 0.0;
        std::string type = "Unknown";
        std::string fileName;
        std::vector<SagData> sagDataPoints;

        bool isValid() const noexcept { return id >= 0; }
        double diameter() const noexcept { return 2.0 * semiDiameter; }

        void clear() noexcept {
            id = -1;
            units = 0;
            sampling = 0;
            angle = 0.0;
            semiDiameter = 0.0;
            type = "Unknown";
            fileName.clear();
            sagDataPoints.clear();
        }
    };

}
