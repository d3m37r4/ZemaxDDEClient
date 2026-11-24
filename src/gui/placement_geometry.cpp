#include "gui/placement_geometry.h"

namespace gui::placement {

    Rect rectFor(const Rect& work, Target target) {
        const int halfW = work.w / 2;
        const int halfH = work.h / 2;
        // The right and bottom parts take the remainder, so the two pieces
        // always add up to the work area on odd sizes.
        const int rightW = work.w - halfW;
        const int bottomH = work.h - halfH;
        switch (target) {
            case Target::Maximize:           return work;
            case Target::LeftHalf:           return {work.x, work.y, halfW, work.h};
            case Target::RightHalf:          return {work.x + halfW, work.y, rightW, work.h};
            case Target::TopLeftQuarter:     return {work.x, work.y, halfW, halfH};
            case Target::TopRightQuarter:    return {work.x + halfW, work.y, rightW, halfH};
            case Target::BottomLeftQuarter:  return {work.x, work.y + halfH, halfW, bottomH};
            case Target::BottomRightQuarter: return {work.x + halfW, work.y + halfH, rightW, bottomH};
        }
        return work;
    }

    std::optional<Target> edgeZoneAt(const Rect& work, int px, int py, int margin) {
        if (px < work.x || px >= work.x + work.w || py < work.y || py >= work.y + work.h) {
            return std::nullopt;
        }
        const bool left = px < work.x + margin;
        const bool right = px >= work.x + work.w - margin;
        const bool top = py < work.y + margin;
        const bool bottom = py >= work.y + work.h - margin;
        if (top && left) return Target::TopLeftQuarter;
        if (top && right) return Target::TopRightQuarter;
        if (bottom && left) return Target::BottomLeftQuarter;
        if (bottom && right) return Target::BottomRightQuarter;
        if (top) return Target::Maximize;
        if (left) return Target::LeftHalf;
        if (right) return Target::RightHalf;
        return std::nullopt;
    }
} // namespace gui::placement
