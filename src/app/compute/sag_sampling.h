#pragma once

namespace app::compute {

    // Limits of the surface profile point count - constraints of the Zemax
    // "Surface Sag Cross Section" tool: fewer than 33 points is rejected by
    // the server, more than 16385 exceeds the tool itself. Applies both to
    // the profile (Surface Profile Inspector) and to the map sections.
    //
    // What this costs in requests is decided by the MAX_MAP_REQUESTS budget:
    // one profile costs (sampling + 2) requests, sections - 180 deg / step.
    constexpr int MIN_SAG_SAMPLING = 33;
    constexpr int MAX_SAG_SAMPLING = 16385;

}
