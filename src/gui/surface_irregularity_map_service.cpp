#include <algorithm>
#include <cmath>
#include <numbers>

#include "surface_irregularity_map_service.h"
#include "gui/settings_manager.h"
#include "gui/utils.h"
#include "lib/implot3d/implot3d.h"

namespace {
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
}

namespace gui {

    void SurfaceIrregularityMapService::renderSurfacePlotLines(const ImVec2& size) {
        int numRadii = (getTargetSampling() + 1) / 2;
        int numAngles = getTotalAngles() * 2;
        int numProfiles = static_cast<int>(getProfiles().size());
        double semiDiameter = getProfiles()[0].semiDiameter;
        double rStep = semiDiameter / (numRadii - 1);
        int centerIdx = (getTargetSampling() - 1) / 2;

        float worstAngle = getMaxPVResult().has_value() ? static_cast<float>(getMaxPVResult()->angle) : -1.0f;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        float sagMin = 1e30f, sagMax = -1e30f;
        for (const auto& profile : getProfiles()) {
            for (const auto& pt : profile.sagDataPoints) {
                float s = static_cast<float>(pt.sag * scaleZ);
                sagMin = std::min(sagMin, s);
                sagMax = std::max(sagMax, s);
            }
        }
        float sagRange = sagMax - sagMin;
        if (sagRange < 1e-12f) sagRange = 1.0f;

        if (ImPlot3D::BeginPlot("##Surface3D_Lines", size, ImPlot3DFlags_NoLegend)) {
            const char* unitX = gui::displayUnitName(m_windowState.units.x);
            const char* unitY = gui::displayUnitName(m_windowState.units.y);
            const char* unitZ = gui::displayUnitName(m_windowState.units.z);
            ImPlot3D::SetupAxes(std::format("X ({})", unitX).c_str(),
                                std::format("Y ({})", unitY).c_str(),
                                std::format("Z ({})", unitZ).c_str());
            ImPlot3D::PushColormap(kColormapNames[m_windowState.selectedColormapSurface]);

            constexpr int SEGMENT_SIZE = 3;
            std::vector<float> xBuf(static_cast<size_t>(numRadii));
            std::vector<float> yBuf(static_cast<size_t>(numRadii));
            std::vector<float> zBuf(static_cast<size_t>(numRadii));

            for (int j = 0; j < numAngles; ++j) {
                int profileIdx = std::min(j / 2, numProfiles - 1);
                bool positiveHalf = (j % 2 == 0);
                double angleRad = profileIdx * std::numbers::pi / numProfiles;
                if (!positiveHalf) angleRad += std::numbers::pi;

                float profileAngle = static_cast<float>(profileIdx * getAngleStepDeg());
                bool isWorst = worstAngle >= 0.0f && std::abs(profileAngle - worstAngle) < 0.01f;

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
                    int numSegments = (numRadii - 1) / SEGMENT_SIZE + 1;
                    for (int seg = 0; seg < numSegments; ++seg) {
                        int start = seg * SEGMENT_SIZE;
                        int end = std::min(start + SEGMENT_SIZE + 1, numRadii);
                        int len = end - start;
                        if (len < 2) continue;

                        int midIdx = start + len / 2;
                        float t = (zBuf[midIdx] - sagMin) / sagRange;
                        ImVec4 color = ImPlot3D::SampleColormap(t);

                        ImPlot3DSpec spec;
                        spec.LineColor = color;
                        spec.LineWeight = 1.5f;
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
        double semiDiameter = getProfiles()[0].semiDiameter;
        double rStep = semiDiameter / (numRadii - 1);
        int centerIdx = (getTargetSampling() - 1) / 2;

        float worstAngle = getMaxPVResult().has_value() ? static_cast<float>(getMaxPVResult()->angle) : -1.0f;

        const double scaleX = gui::unitScaleFactor(m_windowState.units.x);
        const double scaleY = gui::unitScaleFactor(m_windowState.units.y);
        const double scaleZ = gui::unitScaleFactor(m_windowState.units.z);

        float devMin = 1e30f, devMax = -1e30f;
        for (const auto& profile : getProfiles()) {
            for (size_t k = 0; k < profile.sagDataPoints.size() && k < m_nominalSurfaceData.sagDataPoints.size(); ++k) {
                float dev = static_cast<float>((profile.sagDataPoints[k].sag - m_nominalSurfaceData.sagDataPoints[k].sag) * scaleZ);
                devMin = std::min(devMin, dev);
                devMax = std::max(devMax, dev);
            }
        }
        float devRange = devMax - devMin;
        if (devRange < 1e-12f) devRange = 1.0f;

        if (ImPlot3D::BeginPlot("##Deviation3D_Lines", size, ImPlot3DFlags_NoLegend)) {
            const char* unitX = gui::displayUnitName(m_windowState.units.x);
            const char* unitY = gui::displayUnitName(m_windowState.units.y);
            const char* unitZ = gui::displayUnitName(m_windowState.units.z);
            ImPlot3D::SetupAxes(std::format("X ({})", unitX).c_str(),
                                std::format("Y ({})", unitY).c_str(),
                                std::format("ΔSag ({})", unitZ).c_str());
            ImPlot3D::PushColormap(kColormapNames[m_windowState.selectedColormapDeviation]);

            constexpr int SEGMENT_SIZE = 3;
            std::vector<float> xBuf(static_cast<size_t>(numRadii));
            std::vector<float> yBuf(static_cast<size_t>(numRadii));
            std::vector<float> zBuf(static_cast<size_t>(numRadii));

            for (int j = 0; j < numAngles; ++j) {
                int profileIdx = std::min(j / 2, numProfiles - 1);
                bool positiveHalf = (j % 2 == 0);
                double angleRad = profileIdx * std::numbers::pi / numProfiles;
                if (!positiveHalf) angleRad += std::numbers::pi;

                float profileAngle = static_cast<float>(profileIdx * getAngleStepDeg());
                bool isWorst = worstAngle >= 0.0f && std::abs(profileAngle - worstAngle) < 0.01f;

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
                    int numSegments = (numRadii - 1) / SEGMENT_SIZE + 1;
                    for (int seg = 0; seg < numSegments; ++seg) {
                        int start = seg * SEGMENT_SIZE;
                        int end = std::min(start + SEGMENT_SIZE + 1, numRadii);
                        int len = end - start;
                        if (len < 2) continue;

                        int midIdx = start + len / 2;
                        float t = (zBuf[midIdx] - devMin) / devRange;
                        ImVec4 color = ImPlot3D::SampleColormap(t);

                        ImPlot3DSpec spec;
                        spec.LineColor = color;
                        spec.LineWeight = 1.5f;
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
