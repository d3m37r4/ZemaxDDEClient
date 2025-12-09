#pragma once

namespace ZemaxDDE {
    static constexpr const wchar_t* DDE_APP_NAME    = L"ZEMAX";     // Appname DDE
    static constexpr const wchar_t* DDE_TOPIC       = L"RayData";   // Topic DDE
    static constexpr int DDE_TIMEOUT_MS             = 5000;         // Timeout in ms (5 sec.)

    enum class StorageTarget {
        TOLERANCED,
        NOMINAL
    };

    constexpr int MIN_FIELDS = 1;
    constexpr int MAX_FIELDS = 12;

    constexpr int MIN_WAVES  = 1;
    constexpr int MAX_WAVES  = 24;

    constexpr int FIELD_ARRAY_SIZE = MAX_FIELDS + 1;
    constexpr int WAVE_ARRAY_SIZE  = MAX_WAVES + 1;

    constexpr int MAX_SURFACE_NUMBER = 2048;

    /**
     * @brief Codes for command GetSurfaceData/SetSurfaceData
     */
    struct SurfaceDataCode {
        static constexpr int TYPE_NAME             = 0;  // string: Surface type (example, "Standard", "Conic").
        static constexpr int COMMENT               = 1;  // string: Comment.
        static constexpr int CURVATURE             = 2;  // double: Curvature (1/R), not a radius!
        static constexpr int THICKNESS             = 3;  // double: Thickness (mm).
        static constexpr int GLASS                 = 4;  // string: Glass (example, "LZ_K8", "H-K9L" and etc).
        static constexpr int SEMI_DIAMETER         = 5;  // double: Semi-Diameter (mm).
        static constexpr int CONIC                 = 6;  // double: Conic constant.
        static constexpr int COATING               = 7;  // string: Coating.
        static constexpr int TCE                   = 8;  // double: Thermal Coefficient of Expansion.
        static constexpr int DLL_NAME              = 9;  // string: User Defined Surface DLL name.

        static constexpr int IGNORE_SURFACE        = 20; // int: Ignore surface flag. 0 = not ignored, 1 = ignored.

        static constexpr int BEFORE_TILT_DECENTER_ORDER = 51; // int: Before tilt and decenter order. 0=Decenter then Tilt, 1=Tilt then Decenter.
        static constexpr int BEFORE_DECENTER_X     = 52; // double: Before decenter x value.
        static constexpr int BEFORE_DECENTER_Y     = 53; // double: Before decenter y value.
        static constexpr int BEFORE_TILT_X         = 54; // double: Before tilt x value.
        static constexpr int BEFORE_TILT_Y         = 55; // double: Before tilt y value.
        static constexpr int BEFORE_TILT_Z         = 56; // double: Before tilt z value.

        static constexpr int AFTER_STATUS          = 60; // int: After status. 0=explicit, 1=pickup, 2=reverse, 3=pickup this-1, etc.
        static constexpr int AFTER_TILT_DECENTER_ORDER = 61; // int: After tilt and decenter order. 0=Decenter then Tilt, 1=Tilt then Decenter.
        static constexpr int AFTER_DECENTER_X      = 62; // double: After decenter x, decenter y, tilt x, tilt y, and tilt z values, respectively.
        static constexpr int AFTER_DECENTER_Y      = 63; // double: See description for AFTER_DECENTER_X.
        static constexpr int AFTER_TILT_X          = 64; // double: See description for AFTER_DECENTER_X.
        static constexpr int AFTER_TILT_Y          = 65; // double: See description for AFTER_DECENTER_X.
        static constexpr int AFTER_TILT_Z          = 66; // double: See description for AFTER_DECENTER_X.

        static constexpr int USE_LAYER_MULTIPLIERS_AND_INDEX_OFFSETS = 70; // int: Use Layer Multipliers and Index Offsets. Use 1 for true, 0 for false. 1=true, 0=false
        static constexpr int LAYER_MULTIPLIER_VALUE    = 71; // double: Layer Multiplier value. The coating layer number is defined by arg2.
        static constexpr int LAYER_MULTIPLIER_STATUS   = 72; // int: Layer Multiplier status. 0=fixed, 1=variable, n+1=pickup from layer n. The coating layer number is defined by arg2.
        static constexpr int LAYER_INDEX_OFFSET_VALUE  = 73; // double: Layer Index Offset value. The coating layer number is defined by arg2.
        static constexpr int LAYER_INDEX_OFFSET_STATUS = 74; // int: Layer Index Offset status. 0=fixed, 1=variable, n+1=pickup from layer n. The coating layer number is defined by arg2.
        static constexpr int LAYER_EXTINCTION_OFFSET_VALUE  = 75; // double: Layer Extinction Offset value. The coating layer number is defined by arg2.
        static constexpr int LAYER_EXTINCTION_OFFSET_STATUS = 76; // int: Layer Extinction Offset status. 0=fixed, 1=variable, n+1=pickup from layer n. The coating layer number is defined by arg2.

        static constexpr int COORD_RETURN_SOLVE_STATUS = 80; // int: Coordinate return solve status. 0=none, 1=Orientation only, 2=Orient+XY, 3=Orient+XYZ. The surface must be a coordinate break for this solve to have any affect.
        static constexpr int COORD_RETURN_SURFACE_NUMBER = 81; // int: 81 Coordinate return surface number. Used only by the coordinate return solve.

        static bool isValid(int code) {
            switch (code) {
                case TYPE_NAME:
                case COMMENT:
                case CURVATURE:
                case THICKNESS:
                case GLASS:
                case SEMI_DIAMETER:
                case CONIC:
                case COATING:
                case TCE:
                case DLL_NAME:
                case IGNORE_SURFACE:
                case BEFORE_TILT_DECENTER_ORDER:
                case BEFORE_DECENTER_X:
                case BEFORE_DECENTER_Y:
                case BEFORE_TILT_X:
                case BEFORE_TILT_Y:
                case BEFORE_TILT_Z:
                case AFTER_STATUS:
                case AFTER_TILT_DECENTER_ORDER:
                case AFTER_DECENTER_X:
                case AFTER_DECENTER_Y:
                case AFTER_TILT_X:
                case AFTER_TILT_Y:
                case AFTER_TILT_Z:
                case USE_LAYER_MULTIPLIERS_AND_INDEX_OFFSETS:
                case LAYER_MULTIPLIER_VALUE:
                case LAYER_MULTIPLIER_STATUS:
                case LAYER_INDEX_OFFSET_VALUE:
                case LAYER_INDEX_OFFSET_STATUS:
                case LAYER_EXTINCTION_OFFSET_VALUE:
                case LAYER_EXTINCTION_OFFSET_STATUS:
                case COORD_RETURN_SOLVE_STATUS:
                case COORD_RETURN_SURFACE_NUMBER:
                    return true;
                default:
                    return false;
            }
        }
    };
}
