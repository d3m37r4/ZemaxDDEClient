#include <algorithm>
#include <cmath>
#include <numbers>

#include "surface_irregularity_map_service.h"
#include "gui/imgui_utils.h"
#include "gui/plot_image_exporter.h"
#include "app/compute/surface_map.h"
#include "gui/settings_manager.h"
#include "gui/utils.h"
#include "lib/implot3d/implot3d.h"

namespace {
    // ImPlot3D has no public fit request, so a "reset zoom" in the toolbar
    // reproduces the as-created view for one Always-cond frame: the ImPlot3D
    // default InitialRotation quaternion (implot3d_internal.h) and the data
    // extents the plot fit when it was just created.
    const ImPlot3DQuat kDefaultBoxRotation(-0.513269, -0.212596, -0.318184, 0.76819);

    // One Always-cond setup frame; BeginPlot resets RangeCond and RotationCond
    // to None before it, so the lock never outlives the reset frame.
    void resetViewSetup(double xMin, double xMax, double yMin, double yMax,
                        double zMin, double zMax) {
        ImPlot3D::SetupBoxRotation(kDefaultBoxRotation, false, ImPlot3DCond_Always);
        ImPlot3D::SetupAxisLimits(ImAxis3D_X, xMin, xMax, ImPlot3DCond_Always);
        ImPlot3D::SetupAxisLimits(ImAxis3D_Y, yMin, yMax, ImPlot3DCond_Always);
        ImPlot3D::SetupAxisLimits(ImAxis3D_Z, zMin, zMax, ImPlot3DCond_Always);
    }

    ImVec4 imcol(int r, int g, int b) {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    }

    void registerAppColormaps() {
        static bool done = false;
        if (done) return;
        done = true;

        {
            ImVec4 cols[] = { imcol(0x0d, 0x94, 0x88), imcol(0x63, 0x66, 0xf1), imcol(0xa8, 0x55, 0xf7), imcol(0xe8, 0x79, 0xf9) };
            ImPlot3D::AddColormap("Aqua-Purple", cols, 4, false);
        }
        {
            ImVec4 cols[] = { imcol(0x05, 0x96, 0x69), imcol(0x08, 0x91, 0xb2), imcol(0x25, 0x63, 0xeb), imcol(0x7c, 0x3a, 0xed), imcol(0xc0, 0x84, 0xfc) };
            ImPlot3D::AddColormap("Ocean", cols, 5, false);
        }
        {
            ImVec4 cols[] = { imcol(0x14, 0xb8, 0xa6), imcol(0x22, 0xc5, 0x5e), imcol(0xa8, 0x55, 0xf7), imcol(0xec, 0x48, 0x99), imcol(0xef, 0x44, 0x44) };
            ImPlot3D::AddColormap("Aurora", cols, 5, false);
        }
    }

    constexpr const char* kColormapNames[] = { "Cool", "Aqua-Purple", "Ocean", "Aurora" };

    // Wireframe rendering: rings (constant radius) and spokes (constant angle),
    // coloured per vertex from the active colormap so the gradient matches the
    // other modes and the spokes meet cleanly in the centre without a hole or
    // a filled cap.
    void drawWireframe(int numRadii, int numAngles, int numProfiles,
                       double rStep, int stride,
                       double scaleX, double scaleY, double scaleZ,
                       float valueMin, float valueRange,
                       const std::vector<app::models::SurfaceData>& profiles,
                       const app::models::SurfaceData* nominal,
                       int worstIndex, bool highlightWorst, const float* worstColor) {
        std::vector<float> xBuf(static_cast<size_t>(numRadii));
        std::vector<float> yBuf(static_cast<size_t>(numRadii));
        std::vector<float> zBuf(static_cast<size_t>(numRadii));
        std::vector<ImU32> spokeColors(static_cast<size_t>(numRadii));
        std::vector<float> ringX;
        std::vector<float> ringY;
        std::vector<float> ringZ;
        std::vector<ImU32> ringColors;

        const auto sampleColorU32 = [&](float z) {
            const float t = (z - valueMin) / valueRange;
            ImVec4 color = ImPlot3D::SampleColormap(t);
            color.w = 1.0f;
            return ImGui::ColorConvertFloat4ToU32(color);
        };

        // One point of the map: the radius runs from the centre outwards, the
        // angle is fixed.
        const auto pointAt = [&](int angleIndex, int radiusIndex, float& x, float& y, float& z) {
            const int profileIdx = std::min(angleIndex / 2, numProfiles - 1);
            const bool positiveHalf = (angleIndex % 2 == 0);
            double angleRad = profileIdx * std::numbers::pi / numProfiles;
            if (!positiveHalf) angleRad += std::numbers::pi;
            const double r = radiusIndex * rStep;
            x = static_cast<float>(r * std::cos(angleRad) * scaleX);
            y = static_cast<float>(r * std::sin(angleRad) * scaleY);
            const int count = static_cast<int>(profiles[profileIdx].sagDataPoints.size());
            // The centre belongs to the section's own data: a section can end
            // up with fewer points than the sampling setting (a point skipped
            // by a timeout), and the shared index would then shift the whole
            // spoke onto its neighbours.
            const int profileCenter = (count - 1) / 2;
            int srcIdx = positiveHalf ? profileCenter + radiusIndex : profileCenter - radiusIndex;
            srcIdx = std::max(0, std::min(count - 1, srcIdx));
            double value = profiles[profileIdx].sagDataPoints[srcIdx].sag;
            if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                value -= nominal->sagDataPoints[srcIdx].sag;
            }
            z = static_cast<float>(value * scaleZ);
        };

        // Spokes: one polyline per angle from the centre (i = 0) to the rim,
        // shaded per vertex so the centre has the true apex colour.
        for (int j = 0; j < numAngles; ++j) {
            const int profileIdx = std::min(j / 2, numProfiles - 1);
            const bool isWorst = worstIndex >= 0 && profileIdx == worstIndex;
            if (!isWorst && (profileIdx % stride) != 0) {
                continue;
            }

            for (int i = 0; i < numRadii; ++i) {
                pointAt(j, i, xBuf[i], yBuf[i], zBuf[i]);
                spokeColors[i] = sampleColorU32(zBuf[i]);
            }

            ImPlot3DSpec spec;
            if (isWorst && highlightWorst) {
                spec.LineColor = ImVec4(worstColor[0], worstColor[1], worstColor[2], 1.0f);
                spec.LineWeight = 3.0f;
            } else {
                spec.LineColors = spokeColors.data();
                spec.LineWeight = 1.5f;
            }
            char label[16];
            snprintf(label, sizeof(label), "##wsp%d", j);
            ImPlot3D::PlotLine(label, xBuf.data(), yBuf.data(), zBuf.data(), numRadii, spec);
        }

        // Rings: one closed polyline per radius through all kept angles.
        const int ringStep = std::max(1, numRadii / 12);
        for (int i = ringStep; i < numRadii; i += ringStep) {
            ringX.clear();
            ringY.clear();
            ringZ.clear();
            ringColors.clear();
            for (int j = 0; j < numAngles; j += 2) {
                const int profileIdx = std::min(j / 2, numProfiles - 1);
                if ((profileIdx % stride) != 0) continue;
                float x = 0.0f, y = 0.0f, z = 0.0f;
                pointAt(j, i, x, y, z);
                ringX.push_back(x);
                ringY.push_back(y);
                ringZ.push_back(z);
                ringColors.push_back(sampleColorU32(z));
            }
            for (int j = 1; j < numAngles; j += 2) {
                const int profileIdx = std::min(j / 2, numProfiles - 1);
                if ((profileIdx % stride) != 0) continue;
                float x = 0.0f, y = 0.0f, z = 0.0f;
                pointAt(j, i, x, y, z);
                ringX.push_back(x);
                ringY.push_back(y);
                ringZ.push_back(z);
                ringColors.push_back(sampleColorU32(z));
            }
            if (ringX.size() >= 3) {
                ringX.push_back(ringX.front());
                ringY.push_back(ringY.front());
                ringZ.push_back(ringZ.front());
                ringColors.push_back(ringColors.front());
            }
            if (ringX.size() < 2) continue;
            ImPlot3DSpec spec;
            spec.LineColors = ringColors.data();
            spec.LineWeight = 1.0f;
            char label[16];
            snprintf(label, sizeof(label), "##wrg%d", i);
            ImPlot3D::PlotLine(label, ringX.data(), ringY.data(), ringZ.data(),
                               static_cast<int>(ringX.size()), spec);
        }
    }

    // The triangulated surface of the map: one filled mesh, shaded per vertex.
    // ImPlot3D sorts its items against each other, so the surface takes part
    // in the depth ordering on its own - no manual painter's algorithm needed.
    void drawSurfaceMesh(const app::compute::MapMesh& mesh,
                         double scaleX, double scaleY, double scaleZ,
                         float valueMin, float valueRange) {
        if (mesh.empty()) return;
        const size_t vertexCount = mesh.vertexCount();
        std::vector<float> x(vertexCount);
        std::vector<float> y(vertexCount);
        std::vector<float> z(vertexCount);
        std::vector<ImU32> vertexColors(vertexCount);
        for (size_t v = 0; v < vertexCount; ++v) {
            x[v] = static_cast<float>(mesh.x[v] * scaleX);
            y[v] = static_cast<float>(mesh.y[v] * scaleY);
            z[v] = static_cast<float>(mesh.z[v] * scaleZ);
            const float t = (z[v] - valueMin) / valueRange;
            ImVec4 color = ImPlot3D::SampleColormap(t);
            color.w = 1.0f;
            vertexColors[v] = ImGui::ColorConvertFloat4ToU32(color);
        }
        // The fill colours are indexed by position in the index buffer, so a
        // per-vertex shading is copied into the three slots of each triangle.
        std::vector<ImU32> colors(mesh.indices.size());
        for (size_t i = 0; i < mesh.indices.size(); ++i) {
            colors[i] = vertexColors[mesh.indices[i]];
        }

        ImPlot3DSpec spec;
        // Only the filled triangles: the edges of every triangle would be
        // unrolled into the same 16-bit vertex buffer and push the surface
        // over the limit.
        spec.Flags = ImPlot3DMeshFlags_NoLines | ImPlot3DMeshFlags_NoMarkers;
        spec.FillColors = colors.data();
        ImPlot3D::PlotMesh("##mapSurface", x.data(), y.data(), z.data(),
                           mesh.indices.data(), static_cast<int>(vertexCount),
                           static_cast<int>(mesh.indices.size()), spec);
    }

    // The worst section projected into screen pixels across the full diameter
    // (-semiDiameter to +semiDiameter). The filled surface hides a 3D line on
    // the surface because the depth test gives the triangles the tie, so
    // drawing it over the plot keeps the highlight visible end-to-end.
    void collectWorstSectionOverlay(int numRadii, double rStep,
                                    double scaleX, double scaleY, double scaleZ,
                                    int profileIndex,
                                    const std::vector<app::models::SurfaceData>& profiles,
                                    const app::models::SurfaceData* nominal,
                                    std::vector<ImVec2>& out) {
        if (profileIndex < 0 || profileIndex >= static_cast<int>(profiles.size())) return;
        const auto& points = profiles[profileIndex].sagDataPoints;
        if (points.empty()) return;
        const int profileCenter = (static_cast<int>(points.size()) - 1) / 2;

        const double angleRad = profileIndex * std::numbers::pi / static_cast<double>(profiles.size());
        out.clear();
        out.reserve(static_cast<size_t>(2 * numRadii - 1));
        for (int i = -(numRadii - 1); i < numRadii; ++i) {
            const double r = i * rStep;
            const double x = r * std::cos(angleRad) * scaleX;
            const double y = r * std::sin(angleRad) * scaleY;
            int srcIdx = profileCenter + i;
            srcIdx = std::max(0, std::min(static_cast<int>(points.size()) - 1, srcIdx));
            double value = points[srcIdx].sag;
            if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                value -= nominal->sagDataPoints[srcIdx].sag;
            }
            out.push_back(ImPlot3D::PlotToPixels(x, y, value * scaleZ));
        }
    }

    // Draws the projected worst section over the plot.
    void drawOverlayLine(const std::vector<ImVec2>& points, const ImVec4& color,
                         const ImVec2& rectMin, const ImVec2& rectMax) {
        if (points.size() < 2) return;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(rectMin, rectMax, true);
        drawList->AddPolyline(points.data(), static_cast<int>(points.size()),
                              ImGui::ColorConvertFloat4ToU32(color), ImDrawFlags_None, 3.0f);
        drawList->PopClipRect();
    }
}

namespace gui {

    void SurfaceIrregularityMapService::renderSurfacePlotLines(const ImVec2& size) {
        if (getProfiles().empty()) return;
        registerAppColormaps();

        int numRadii = (getTargetSampling() + 1) / 2;
        int numProfiles = static_cast<int>(getProfiles().size());
        // Cached extents: recomputed only when the data changes instead of
        // walking every point of every section on every frame.
        const auto& ranges = dataRanges();
        double semiDiameter = ranges.semiDiameter;
        double rStep = semiDiameter / (numRadii - 1);
        if (!(rStep > 0.0)) {
            // No radius in the data: every section would collapse onto the Z
            // axis, so there is nothing meaningful to draw.
            return;
        }
        // The worst section is addressed by its index: matching it by angle
        // breaks as soon as the angle rounding or the step differs.
        const int worstIndex = getMaxPVResult().has_value() ? getMaxPVResult()->profileIndex : -1;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        const float sagMin = ranges.sagValid ? ranges.sagMin * static_cast<float>(scaleZ) : 0.0f;
        const float sagMax = ranges.sagValid ? ranges.sagMax * static_cast<float>(scaleZ) : 1.0f;
        float sagRange = sagMax - sagMin;
        if (sagRange < 1e-12f) sagRange = 1.0f;

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();

        if (ImPlot3D::BeginPlot("##Surface3D_Lines", size, ImPlot3DFlags_NoLegend)) {
            const int numAngles = getTotalAngles() * 2;
            // The worst section of the surface, projected over the plot.
            std::vector<ImVec2> overlay;
            ImVec2 overlayRectMin(0.0f, 0.0f);
            ImVec2 overlayRectMax(0.0f, 0.0f);
            const char* unitX = gui::displayUnitName(m_windowState.units.x);
            const char* unitY = gui::displayUnitName(m_windowState.units.y);
            const char* unitZ = gui::displayUnitName(m_windowState.units.z);
            ImPlot3D::SetupAxes(std::format("X ({})", unitX).c_str(),
                                std::format("Y ({})", unitY).c_str(),
                                std::format("Z ({})", unitZ).c_str());
            ImGuiUtils::SetPlotFrameSize(ImPlot3D::GetPlotRectSize());
            if (plotRequest == ImGuiUtils::kPlotRequestReset) {
                resetViewSetup(-semiDiameter * scaleX, semiDiameter * scaleX,
                               -semiDiameter * scaleY, semiDiameter * scaleY,
                               sagMin, sagMax);
            }
            if (plotRequest == ImGuiUtils::kPlotRequestSaveImage) {
                plot_export::requestSave(m_logger, "map", m_nominalSurfaceData,
                                         ImGui::GetWindowDrawList(),
                                         ImPlot3D::GetPlotRectPos(), ImPlot3D::GetPlotRectSize(),
                                         ImGuiUtils::PlotExportResolution());
            }
            const int cmapIdx = std::clamp(m_windowState.selectedColormapSurface,
                                           0, static_cast<int>(std::size(kColormapNames)) - 1);
            ImPlot3D::PushColormap(kColormapNames[cmapIdx]);

            const bool wireframe = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Wireframe);
            const bool surface = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Surface);

            // A precise map (thousands of sections) submits hundreds of
            // thousands of plot items per frame and overruns the 16-bit
            // ImDrawIdx range. Thin the drawn set by an item budget; the
            // wireframe submits two spokes per section.
            constexpr int kMaxDrawnItems = 1200;
            const int stride = app::compute::drawnProfileStride(
                numProfiles, 2, kMaxDrawnItems);

            if (wireframe) {
                drawWireframe(numRadii, numAngles, numProfiles, rStep,
                              stride, scaleX, scaleY, scaleZ, sagMin, sagRange,
                              getProfiles(), nullptr, worstIndex,
                              m_windowState.highlightWorstSurface,
                              m_windowState.worstColorSurface);
            } else if (surface) {
                drawSurfaceMesh(mapMesh(false), scaleX, scaleY, scaleZ, sagMin, sagRange);
            }

            // A section is stored as one full cross-section (-R..+R), so it is
            // drawn as a single line through the centre, exactly as measured.
            // Splitting it into two rays under theta and theta+180 made the map
            // read as two mirrored halves, and 360 nearly coincident lines beat
            // into a moire instead of a surface. The number of points follows
            // the data (sampling setting, minus any point skipped by a timeout),
            // so nothing here depends on a fixed count.
            size_t maxPoints = 2;
            for (const auto& profile : getProfiles()) {
                maxPoints = std::max(maxPoints, profile.sagDataPoints.size());
            }
            std::vector<float> xBuf(maxPoints);
            std::vector<float> yBuf(maxPoints);
            std::vector<float> zBuf(maxPoints);
            std::vector<ImU32> lineColors(maxPoints);

            const int lineStride = app::compute::drawnProfileStride(
                numProfiles, 1, kMaxDrawnItems);

            for (int k = 0; k < numProfiles && !wireframe && !surface; ++k) {
                const bool isWorst = worstIndex >= 0 && k == worstIndex;
                if (!isWorst && (k % lineStride) != 0) {
                    continue;
                }
                const auto& points = getProfiles()[k].sagDataPoints;
                const int count = static_cast<int>(points.size());
                if (count < 2) {
                    continue;
                }

                for (int i = 0; i < count; ++i) {
                    xBuf[i] = static_cast<float>(points[i].x * scaleX);
                    yBuf[i] = static_cast<float>(points[i].y * scaleY);
                    zBuf[i] = static_cast<float>(points[i].sag * scaleZ);

                    const float t = (zBuf[i] - sagMin) / sagRange;
                    ImVec4 color = ImPlot3D::SampleColormap(t);
                    color.w = 1.0f;
                    lineColors[i] = ImGui::ColorConvertFloat4ToU32(color);
                }

                ImPlot3DSpec spec;
                if (isWorst && m_windowState.highlightWorstSurface) {
                    spec.LineColor = ImVec4(m_windowState.worstColorSurface[0], m_windowState.worstColorSurface[1], m_windowState.worstColorSurface[2], 1.0f);
                    spec.LineWeight = 3.0f;
                } else {
                    spec.LineColors = lineColors.data();
                    spec.LineWeight = 2.0f;
                }
                char label[24];
                snprintf(label, sizeof(label), "##sp%d", k);
                ImPlot3D::PlotLine(label, xBuf.data(), yBuf.data(), zBuf.data(), count, spec);
            }

            // Collect the full-diameter worst-section overlay after the plot
            // items have been submitted so the auto-fitted axis ranges are up
            // to date when PlotToPixels projects the points.
            if (m_windowState.highlightWorstSurface && worstIndex >= 0) {
                const ImVec2 rectSize = ImPlot3D::GetPlotRectSize();
                overlayRectMin = ImPlot3D::GetPlotRectPos();
                overlayRectMax = ImVec2(overlayRectMin.x + rectSize.x,
                                        overlayRectMin.y + rectSize.y);
                collectWorstSectionOverlay(numRadii, rStep,
                                           scaleX, scaleY, scaleZ, worstIndex,
                                           getProfiles(), nullptr, overlay);
            }

            ImPlot3D::PopColormap();
            ImPlot3D::EndPlot();
            drawOverlayLine(overlay,
                            ImVec4(m_windowState.worstColorSurface[0], m_windowState.worstColorSurface[1],
                                   m_windowState.worstColorSurface[2], 1.0f),
                            overlayRectMin, overlayRectMax);
        }
    }

    void SurfaceIrregularityMapService::renderDeviationSurfacePlotLines(const ImVec2& size) {
        if (getProfiles().empty() || !m_nominalSurfaceData.isValid()) return;
        registerAppColormaps();

        int numRadii = (getTargetSampling() + 1) / 2;
        int numProfiles = static_cast<int>(getProfiles().size());
        // Cached deviation extents: recomputed only when the data changes instead
        // of walking every point of every section on every frame.
        const auto& ranges = dataRanges();
        double semiDiameter = ranges.semiDiameter;
        double rStep = semiDiameter / (numRadii - 1);
        if (!(rStep > 0.0)) {
            // No radius in the data: every section would collapse onto the Z
            // axis, so there is nothing meaningful to draw.
            return;
        }
        // The worst section is addressed by its index: matching it by angle
        // breaks as soon as the angle rounding or the step differs.
        const int worstIndex = getMaxPVResult().has_value() ? getMaxPVResult()->profileIndex : -1;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        const float devMin = ranges.devValid ? ranges.devMin * static_cast<float>(scaleZ) : 0.0f;
        const float devMax = ranges.devValid ? ranges.devMax * static_cast<float>(scaleZ) : 1.0f;
        float devRange = devMax - devMin;
        if (devRange < 1e-12f) devRange = 1.0f;

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();

        if (ImPlot3D::BeginPlot("##Deviation3D_Lines", size, ImPlot3DFlags_NoLegend)) {
            const int numAngles = getTotalAngles() * 2;
            // The worst section of the surface, projected over the plot.
            std::vector<ImVec2> overlay;
            ImVec2 overlayRectMin(0.0f, 0.0f);
            ImVec2 overlayRectMax(0.0f, 0.0f);
            const char* unitX = gui::displayUnitName(m_windowState.units.x);
            const char* unitY = gui::displayUnitName(m_windowState.units.y);
            const char* unitZ = gui::displayUnitName(m_windowState.units.z);
            ImPlot3D::SetupAxes(std::format("X ({})", unitX).c_str(),
                                std::format("Y ({})", unitY).c_str(),
                                std::format("ΔSag ({})", unitZ).c_str());
            ImGuiUtils::SetPlotFrameSize(ImPlot3D::GetPlotRectSize());
            if (plotRequest == ImGuiUtils::kPlotRequestReset) {
                resetViewSetup(-semiDiameter * scaleX, semiDiameter * scaleX,
                               -semiDiameter * scaleY, semiDiameter * scaleY,
                               devMin, devMax);
            }
            if (plotRequest == ImGuiUtils::kPlotRequestSaveImage) {
                plot_export::requestSave(m_logger, "devmap", m_nominalSurfaceData,
                                         ImGui::GetWindowDrawList(),
                                         ImPlot3D::GetPlotRectPos(), ImPlot3D::GetPlotRectSize(),
                                         ImGuiUtils::PlotExportResolution());
            }
            const int cmapIdx = std::clamp(m_windowState.selectedColormapDeviation,
                                           0, static_cast<int>(std::size(kColormapNames)) - 1);
            ImPlot3D::PushColormap(kColormapNames[cmapIdx]);

            const bool wireframe = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Wireframe);
            const bool surface = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Surface);

            // Same thinning as the surface plot: the wireframe submits two
            // spokes per section.
            constexpr int kMaxDrawnItems = 1200;
            const int stride = app::compute::drawnProfileStride(
                numProfiles, 2, kMaxDrawnItems);

            if (wireframe) {
                drawWireframe(numRadii, numAngles, numProfiles, rStep,
                              stride, scaleX, scaleY, scaleZ, devMin, devRange,
                              getProfiles(), &m_nominalSurfaceData, worstIndex,
                              m_windowState.highlightWorstDeviation,
                              m_windowState.worstColorDeviation);
            } else if (surface) {
                drawSurfaceMesh(mapMesh(true), scaleX, scaleY, scaleZ, devMin, devRange);
            }

            // Same as the surface plot: one measured cross-section per line,
            // drawn through the centre, with the point count taken from the
            // data instead of the sampling setting.
            size_t maxPoints = 2;
            for (const auto& profile : getProfiles()) {
                maxPoints = std::max(maxPoints, profile.sagDataPoints.size());
            }
            std::vector<float> xBuf(maxPoints);
            std::vector<float> yBuf(maxPoints);
            std::vector<float> zBuf(maxPoints);
            std::vector<ImU32> lineColors(maxPoints);

            const int lineStride = app::compute::drawnProfileStride(
                numProfiles, 1, kMaxDrawnItems);

            for (int k = 0; k < numProfiles && !wireframe && !surface; ++k) {
                const bool isWorst = worstIndex >= 0 && k == worstIndex;
                if (!isWorst && (k % lineStride) != 0) {
                    continue;
                }
                const auto& points = getProfiles()[k].sagDataPoints;
                const int count = static_cast<int>(points.size());
                if (count < 2) {
                    continue;
                }

                for (int i = 0; i < count; ++i) {
                    xBuf[i] = static_cast<float>(points[i].x * scaleX);
                    yBuf[i] = static_cast<float>(points[i].y * scaleY);

                    float sag = static_cast<float>(points[i].sag);
                    if (i < static_cast<int>(m_nominalSurfaceData.sagDataPoints.size())) {
                        sag -= static_cast<float>(m_nominalSurfaceData.sagDataPoints[i].sag);
                    }
                    zBuf[i] = sag * static_cast<float>(scaleZ);

                    const float t = (zBuf[i] - devMin) / devRange;
                    ImVec4 color = ImPlot3D::SampleColormap(t);
                    color.w = 1.0f;
                    lineColors[i] = ImGui::ColorConvertFloat4ToU32(color);
                }

                ImPlot3DSpec spec;
                if (isWorst && m_windowState.highlightWorstDeviation) {
                    spec.LineColor = ImVec4(m_windowState.worstColorDeviation[0], m_windowState.worstColorDeviation[1], m_windowState.worstColorDeviation[2], 1.0f);
                    spec.LineWeight = 3.0f;
                } else {
                    spec.LineColors = lineColors.data();
                    spec.LineWeight = 2.0f;
                }
                char label[24];
                snprintf(label, sizeof(label), "##dp%d", k);
                ImPlot3D::PlotLine(label, xBuf.data(), yBuf.data(), zBuf.data(), count, spec);
            }

            if (m_windowState.highlightWorstDeviation && worstIndex >= 0) {
                const ImVec2 rectSize = ImPlot3D::GetPlotRectSize();
                overlayRectMin = ImPlot3D::GetPlotRectPos();
                overlayRectMax = ImVec2(overlayRectMin.x + rectSize.x,
                                        overlayRectMin.y + rectSize.y);
                collectWorstSectionOverlay(numRadii, rStep,
                                           scaleX, scaleY, scaleZ, worstIndex,
                                           getProfiles(), &m_nominalSurfaceData, overlay);
            }

            ImPlot3D::PopColormap();
            ImPlot3D::EndPlot();
            drawOverlayLine(overlay,
                            ImVec4(m_windowState.worstColorDeviation[0], m_windowState.worstColorDeviation[1],
                                   m_windowState.worstColorDeviation[2], 1.0f),
                            overlayRectMin, overlayRectMax);
        }
    }
}
