#pragma once

#include "app/models/optical_system.h"
#include "app/models/types.h"

namespace ZemaxDDE {

    // Re-export domain models from app/models/ for backward compatibility.
    // New code should use app::models:: directly.
    using app::models::OpticalSystemData;
    using app::models::ConnectionState;
    using app::models::toString;

}
