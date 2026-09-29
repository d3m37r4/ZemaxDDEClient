#pragma once

#include <cstdint>
#include <string>

namespace app {
    std::string formatFileSize(uintmax_t bytes);

    /// Clamps value to [lo, hi] in place.
    /// @return true when the value was rewritten (drives "adjusted" UI feedback).
    template <typename T>
    bool ClampWithFeedback(T& value, T lo, T hi) {
        if (value < lo) {
            value = lo;
            return true;
        }
        if (value > hi) {
            value = hi;
            return true;
        }
        return false;
    }

    /// Snaps an int up to the nearest odd number in place.
    /// @return true when the value was rewritten.
    inline bool SnapOddWithFeedback(int& value) {
        const int snapped = value | 1;
        if (snapped != value) {
            value = snapped;
            return true;
        }
        return false;
    }
}
