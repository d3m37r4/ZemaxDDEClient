#pragma once

namespace app::models {

    enum class TaskSource {
        None,
        NominalSurfaceProfile,
        TolerancedSurfaceProfile,
        SurfaceIrregularityMap
    };

    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected
    };

    constexpr const char* toString(ConnectionState s) noexcept {
        switch (s) {
            case ConnectionState::Disconnected: return "Disconnected";
            case ConnectionState::Connecting:   return "Connecting";
            case ConnectionState::Connected:    return "Connected";
        }
        return "Unknown";
    }

    struct MaxPVResult {
        double angle;
        double peak;
        double valley;
        double pv;
    };

}
