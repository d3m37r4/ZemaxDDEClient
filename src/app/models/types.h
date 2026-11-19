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
        /// Index of the section in the profile list. The highlight of the worst
        /// section is drawn by index: matching it by angle breaks as soon as
        /// the angle rounding or the step differs.
        int profileIndex = -1;
    };

}
