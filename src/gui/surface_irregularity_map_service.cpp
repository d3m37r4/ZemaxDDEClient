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

    // Wireframe rendering: rings (constant radius) and spokes (constant angle)
    // instead of the colour-banded profile segments. Both maps share it - the
    // surface map uses the sag, the deviation map the difference to the
    // nominal surface.
    void drawWireframe(int numRadii, int numAngles, int numProfiles,
                       double rStep, int centerIdx, double angleStepDeg,
                       double scaleX, double scaleY,
                       float valueMin, float valueRange,
                       const std::vector<app::models::SurfaceData>& profiles,
                       const app::models::SurfaceData* nominal,
                       float worstAngle, bool highlightWorst, const float* worstColor) {
        std::vector<float> xBuf(static_cast<size_t>(numRadii));
        std::vector<float> yBuf(static_cast<size_t>(numRadii));
        std::vector<float> zBuf(static_cast<size_t>(numRadii));
        std::vector<float> ringX;
        std::vector<float> ringY;
        std::vector<float> ringZ;

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
            int srcIdx = positiveHalf ? centerIdx + radiusIndex : centerIdx - radiusIndex;
            srcIdx = std::max(0, std::min(count - 1, srcIdx));
            float value = static_cast<float>(profiles[profileIdx].sagDataPoints[srcIdx].sag);
            if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                value -= static_cast<float>(nominal->sagDataPoints[srcIdx].sag);
            }
            z = value;
        };

        // Spokes: one polyline per angle. They start at the first non-zero
        // radius - all spokes meet in the centre, and the colour of that
        // meeting point painted a blob there.
        for (int j = 0; j < numAngles; ++j) {
            for (int i = 1; i < numRadii; ++i) {
                pointAt(j, i, xBuf[i - 1], yBuf[i - 1], zBuf[i - 1]);
            }
            const int profileIdx = std::min(j / 2, numProfiles - 1);
            const float profileAngle = static_cast<float>(profileIdx * angleStepDeg);
            const bool isWorst = worstAngle >= 0.0f && std::abs(profileAngle - worstAngle) < 0.01f;

            ImPlot3DSpec spec;
            if (isWorst && highlightWorst) {
                spec.LineColor = ImVec4(worstColor[0], worstColor[1], worstColor[2], 1.0f);
                spec.LineWeight = 3.0f;
            } else {
                const float t = (zBuf[numRadii / 2] - valueMin) / valueRange;
                ImVec4 color = ImPlot3D::SampleColormap(t);
                color.w = 1.0f;
                spec.LineColor = color;
                spec.LineWeight = 2.0f;
            }
            char label[16];
            snprintf(label, sizeof(label), "##wsp%d", j);
            ImPlot3D::PlotLine(label, xBuf.data(), yBuf.data(), zBuf.data(), numRadii - 1, spec);
        }

        // Rings: one polyline per radius, through every angle. The positive
        // halves run forward and the negative halves back, so the polyline
        // closes into a circle instead of zig-zagging across the centre.
        const int ringStep = std::max(1, numRadii / 12);
        for (int i = ringStep; i < numRadii; i += ringStep) {
            ringX.clear();
            ringY.clear();
            ringZ.clear();
            for (int j = 0; j < numAngles; j += 2) {
                float x = 0.0f, y = 0.0f, z = 0.0f;
                pointAt(j, i, x, y, z);
                ringX.push_back(x);
                ringY.push_back(y);
                ringZ.push_back(z);
            }
            for (int j = numAngles - 1; j >= 1; j -= 2) {
                float x = 0.0f, y = 0.0f, z = 0.0f;
                pointAt(j, i, x, y, z);
                ringX.push_back(x);
                ringY.push_back(y);
                ringZ.push_back(z);
            }
            const float t = (ringZ[ringZ.size() / 2] - valueMin) / valueRange;
            ImVec4 color = ImPlot3D::SampleColormap(t);
            color.w = 1.0f;
            ImPlot3DSpec spec;
            spec.LineColor = color;
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
        spec.FillColors = colors.data();
        spec.LineColors = colors.data();
        spec.LineWeight = 1.0f;
        ImPlot3D::PlotMesh("##mapSurface", x.data(), y.data(), z.data(),
                           mesh.indices.data(), static_cast<int>(vertexCount),
                           static_cast<int>(mesh.indices.size()), spec);
    }

    // The worst section on top of the surface: the highlighted profile as a
    // line, so the worst spot stays visible in the filled rendering too.
    void drawWorstSection(int numRadii, int numProfiles, int centerIdx, double rStep,
                          double angleStepDeg, double scaleX, double scaleY, double scaleZ,
                          float worstAngle, const float* worstColor,
                          const std::vector<app::models::SurfaceData>& profiles,
                          const app::models::SurfaceData* nominal) {
        if (worstAngle < 0.0f || worstColor == nullptr) return;
        const int profileIdx = static_cast<int>(std::lround(worstAngle / angleStepDeg));
        if (profileIdx < 0 || profileIdx >= numProfiles) return;
        const auto& points = profiles[profileIdx].sagDataPoints;
        if (points.empty()) return;

        const double angleRad = profileIdx * std::numbers::pi / numProfiles;
        std::vector<float> x(static_cast<size_t>(numRadii));
        std::vector<float> y(static_cast<size_t>(numRadii));
        std::vector<float> z(static_cast<size_t>(numRadii));
        for (int i = 0; i < numRadii; ++i) {
            const double r = i * rStep;
            x[i] = static_cast<float>(r * std::cos(angleRad) * scaleX);
            y[i] = static_cast<float>(r * std::sin(angleRad) * scaleY);
            int srcIdx = centerIdx + i;
            srcIdx = std::max(0, std::min(static_cast<int>(points.size()) - 1, srcIdx));
            float value = static_cast<float>(points[srcIdx].sag);
            if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                value -= static_cast<float>(nominal->sagDataPoints[srcIdx].sag);
            }
            z[i] = static_cast<float>(value * scaleZ);
        }

        ImPlot3DSpec spec;
        spec.LineColor = ImVec4(worstColor[0], worstColor[1], worstColor[2], 1.0f);
        spec.LineWeight = 3.0f;
        ImPlot3D::PlotLine("##worstSection", x.data(), y.data(), z.data(), numRadii, spec);
    }
}

namespace gui {

    void SurfaceIrregularityMapService::renderSurfacePlotLines(const ImVec2& size) {
        if (getProfiles().empty()) return;
        int numRadii = (getTargetSampling() + 1) / 2;
        int numAngles = getTotalAngles() * 2;
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
        int centerIdx = (getTargetSampling() - 1) / 2;

        float worstAngle = getMaxPVResult().has_value() ? static_cast<float>(getMaxPVResult()->angle) : -1.0f;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        const float sagMin = ranges.sagValid ? ranges.sagMin * static_cast<float>(scaleZ) : 0.0f;
        const float sagMax = ranges.sagValid ? ranges.sagMax * static_cast<float>(scaleZ) : 1.0f;
        float sagRange = sagMax - sagMin;
        if (sagRange < 1e-12f) sagRange = 1.0f;

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();

        if (ImPlot3D::BeginPlot("##Surface3D_Lines", size, ImPlot3DFlags_NoLegend)) {
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
            ImPlot3D::PushColormap(kColormapNames[m_windowState.selectedColormapSurface]);

            const bool wireframe = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Wireframe);
            const bool surface = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Surface);
            if (wireframe) {
                drawWireframe(numRadii, numAngles, numProfiles, rStep, centerIdx,
                              getAngleStepDeg(), scaleX, scaleY, sagMin, sagRange,
                              getProfiles(), nullptr, worstAngle,
                              m_windowState.highlightWorstSurface,
                              m_windowState.worstColorSurface);
            } else if (surface) {
                drawSurfaceMesh(mapMesh(false), scaleX, scaleY, scaleZ, sagMin, sagRange);
                if (m_windowState.highlightWorstSurface) {
                    drawWorstSection(numRadii, numProfiles, centerIdx, rStep,
                                     getAngleStepDeg(), scaleX, scaleY, scaleZ, worstAngle,
                                     m_windowState.worstColorSurface, getProfiles(), nullptr);
                }
            }

            constexpr int SEGMENT_SIZE = 3;
            const int numSegments = (numRadii - 1) / SEGMENT_SIZE + 1;
            // A precise map (thousands of sections) submits hundreds of
            // thousands of plot items per frame and overruns the 16-bit
            // ImDrawIdx range. Thin the drawn set by an item budget - the
            // items are what floods ImPlot3D, not the sections, and counting
            // sections left the thinning off exactly when the map grew. The
            // worst section always stays so the highlight never disappears.
            // The underlying data keeps its full resolution.
            constexpr int kMaxDrawnItems = 1200;
            const int stride = app::compute::drawnProfileStride(
                numProfiles, 2 * numSegments, kMaxDrawnItems);
            std::vector<float> xBuf(static_cast<size_t>(numRadii));
            std::vector<float> yBuf(static_cast<size_t>(numRadii));
            std::vector<float> zBuf(static_cast<size_t>(numRadii));

            for (int j = 0; j < numAngles && !wireframe && !surface; ++j) {
                int profileIdx = std::min(j / 2, numProfiles - 1);
                bool positiveHalf = (j % 2 == 0);
                double angleRad = profileIdx * std::numbers::pi / numProfiles;
                if (!positiveHalf) angleRad += std::numbers::pi;

                float profileAngle = static_cast<float>(profileIdx * getAngleStepDeg());
                bool isWorst = worstAngle >= 0.0f && std::abs(profileAngle - worstAngle) < 0.01f;
                if (!isWorst && (profileIdx % stride) != 0) {
                    continue;
                }

                for (int i = 0; i < numRadii; ++i) {
                    double r = i * rStep;
                    xBuf[i] = static_cast<float>(r * std::cos(angleRad) * scaleX);
                    yBuf[i] = static_cast<float>(r * std::sin(angleRad) * scaleY);

                    int srcIdx = positiveHalf ? centerIdx + i : centerIdx - i;
                    srcIdx = std::max(0, std::min(static_cast<int>(getProfiles()[profileIdx].sagDataPoints.size()) - 1, srcIdx));
                    zBuf[i] = static_cast<float>(getProfiles()[profileIdx].sagDataPoints[srcIdx].sag * scaleZ);
                }

                if (isWorst && m_windowState.highlightWorstSurface) {
                    ImPlot3DSpec spec;
                    spec.LineColor = ImVec4(m_windowState.worstColorSurface[0], m_windowState.worstColorSurface[1], m_windowState.worstColorSurface[2], 1.0f);
                    spec.LineWeight = 3.0f;
                    char worstLabel[16];
                    snprintf(worstLabel, sizeof(worstLabel), "##sw%d", j);
                    ImPlot3D::PlotLine(worstLabel, xBuf.data(), yBuf.data(), zBuf.data(), numRadii, spec);
                } else {
                    for (int seg = 0; seg < numSegments; ++seg) {
                        int start = seg * SEGMENT_SIZE;
                        int end = std::min(start + SEGMENT_SIZE + 1, numRadii);
                        int len = end - start;
                        if (len < 2) continue;

                        int midIdx = start + len / 2;
                        float t = (zBuf[midIdx] - sagMin) / sagRange;
                        ImVec4 color = ImPlot3D::SampleColormap(t);
                        color.w = 1.0f;   // the colormap must not fade the sections out

                        ImPlot3DSpec spec;
                        spec.LineColor = color;
                        spec.LineWeight = 2.0f;
                        char segLabel[32];
                        snprintf(segLabel, sizeof(segLabel), "##sg%d_%d", j, seg);
                        ImPlot3D::PlotLine(segLabel, xBuf.data() + start, yBuf.data() + start, zBuf.data() + start, len, spec);
                    }
                }
            }

            ImPlot3D::PopColormap();
            ImPlot3D::EndPlot();
        }
    }

    void SurfaceIrregularityMapService::renderDeviationSurfacePlotLines(const ImVec2& size) {
        if (getProfiles().empty() || !m_nominalSurfaceData.isValid()) return;
        registerAppColormaps();

        int numRadii = (getTargetSampling() + 1) / 2;
        int numAngles = getTotalAngles() * 2;
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
        int centerIdx = (getTargetSampling() - 1) / 2;

        float worstAngle = getMaxPVResult().has_value() ? static_cast<float>(getMaxPVResult()->angle) : -1.0f;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        const float devMin = ranges.devValid ? ranges.devMin * static_cast<float>(scaleZ) : 0.0f;
        const float devMax = ranges.devValid ? ranges.devMax * static_cast<float>(scaleZ) : 1.0f;
        float devRange = devMax - devMin;
        if (devRange < 1e-12f) devRange = 1.0f;

        const int plotRequest = ImGuiUtils::ConsumePlotRequest();

        if (ImPlot3D::BeginPlot("##Deviation3D_Lines", size, ImPlot3DFlags_NoLegend)) {
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
            ImPlot3D::PushColormap(kColormapNames[m_windowState.selectedColormapDeviation]);

            const bool wireframe = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Wireframe);
            const bool surface = m_windowState.renderMode ==
                static_cast<int>(app::services::MapRenderMode::Surface);
            if (wireframe) {
                drawWireframe(numRadii, numAngles, numProfiles, rStep, centerIdx,
                              getAngleStepDeg(), scaleX, scaleY, devMin, devRange,
                              getProfiles(), &m_nominalSurfaceData, worstAngle,
                              m_windowState.highlightWorstDeviation,
                              m_windowState.worstColorDeviation);
            } else if (surface) {
                drawSurfaceMesh(mapMesh(true), scaleX, scaleY, scaleZ, devMin, devRange);
                if (m_windowState.highlightWorstDeviation) {
                    drawWorstSection(numRadii, numProfiles, centerIdx, rStep,
                                     getAngleStepDeg(), scaleX, scaleY, scaleZ, worstAngle,
                                     m_windowState.worstColorDeviation, getProfiles(),
                                     &m_nominalSurfaceData);
                }
            }

            constexpr int SEGMENT_SIZE = 3;
            const int numSegments = (numRadii - 1) / SEGMENT_SIZE + 1;
            // Same thinning as the surface plot: the item budget is what keeps
            // ImPlot3D from being flooded every frame. The worst section is
            // always kept so its highlight never disappears.
            constexpr int kMaxDrawnItems = 1200;
            const int stride = app::compute::drawnProfileStride(
                numProfiles, 2 * numSegments, kMaxDrawnItems);
            std::vector<float> xBuf(static_cast<size_t>(numRadii));
            std::vector<float> yBuf(static_cast<size_t>(numRadii));
            std::vector<float> zBuf(static_cast<size_t>(numRadii));

            for (int j = 0; j < numAngles && !wireframe && !surface; ++j) {
                int profileIdx = std::min(j / 2, numProfiles - 1);
                bool positiveHalf = (j % 2 == 0);
                double angleRad = profileIdx * std::numbers::pi / numProfiles;
                if (!positiveHalf) angleRad += std::numbers::pi;

                float profileAngle = static_cast<float>(profileIdx * getAngleStepDeg());
                bool isWorst = worstAngle >= 0.0f && std::abs(profileAngle - worstAngle) < 0.01f;
                if (!isWorst && (profileIdx % stride) != 0) {
                    continue;
                }

                for (int i = 0; i < numRadii; ++i) {
                    double r = i * rStep;
                    xBuf[i] = static_cast<float>(r * std::cos(angleRad) * scaleX);
                    yBuf[i] = static_cast<float>(r * std::sin(angleRad) * scaleY);

                    int srcIdx = positiveHalf ? centerIdx + i : centerIdx - i;
                    srcIdx = std::max(0, std::min(static_cast<int>(getProfiles()[profileIdx].sagDataPoints.size()) - 1, srcIdx));

                    float sag = static_cast<float>(getProfiles()[profileIdx].sagDataPoints[srcIdx].sag);
                    if (srcIdx < static_cast<int>(m_nominalSurfaceData.sagDataPoints.size())) {
                        sag -= static_cast<float>(m_nominalSurfaceData.sagDataPoints[srcIdx].sag);
                    }
                    zBuf[i] = sag * static_cast<float>(scaleZ);
                }

                if (isWorst && m_windowState.highlightWorstDeviation) {
                    ImPlot3DSpec spec;
                    spec.LineColor = ImVec4(m_windowState.worstColorDeviation[0], m_windowState.worstColorDeviation[1], m_windowState.worstColorDeviation[2], 1.0f);
                    spec.LineWeight = 3.0f;
                    char worstLabel[16];
                    snprintf(worstLabel, sizeof(worstLabel), "##dw%d", j);
                    ImPlot3D::PlotLine(worstLabel, xBuf.data(), yBuf.data(), zBuf.data(), numRadii, spec);
                } else {
                    for (int seg = 0; seg < numSegments; ++seg) {
                        int start = seg * SEGMENT_SIZE;
                        int end = std::min(start + SEGMENT_SIZE + 1, numRadii);
                        int len = end - start;
                        if (len < 2) continue;

                        int midIdx = start + len / 2;
                        float t = (zBuf[midIdx] - devMin) / devRange;
                        ImVec4 color = ImPlot3D::SampleColormap(t);
                        color.w = 1.0f;   // the colormap must not fade the sections out

                        ImPlot3DSpec spec;
                        spec.LineColor = color;
                        spec.LineWeight = 2.0f;
                        char segLabel[32];
                        snprintf(segLabel, sizeof(segLabel), "##dg%d_%d", j, seg);
                        ImPlot3D::PlotLine(segLabel, xBuf.data() + start, yBuf.data() + start, zBuf.data() + start, len, spec);
                    }
                }
            }

            ImPlot3D::PopColormap();
            ImPlot3D::EndPlot();
        }
    }
}
