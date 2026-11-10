#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "app/models/types.h"
#include "app/services/operation_monitor_service.h"
#include "app/compute/surface_profile.h"
#include "app/compute/surface_map.h"
#include "app/services/surface_profile_service.h"

class Logger;

namespace app::services {

    // The guard rails of a surface map (step and sampling limits, request budget)
    // live in app::compute::surface_map.h — the service and the UI share them.

    struct MapWindowState {
        int nominalSurfaceIndex = 0;
        int nominalSampling = 65;
        double nominalAngle = 0.0;

        int tolerancedSurfaceIndex = 0;
        int tolerancedSampling = 65;
        double tolerancedAngleStep = 1.0;

        int selectedColormapSurface = 0;
        int selectedColormapDeviation = 0;

        bool highlightWorstSurface = true;
        bool highlightWorstDeviation = true;
        float worstColorSurface[3] = {1.0f, 0.0f, 0.0f};
        float worstColorDeviation[3] = {1.0f, 0.0f, 0.0f};

        // Same "value adjusted" flash timestamps as ProfileWindowState.
        double nominalSurfaceAdjustedAt = 0.0;
        double nominalSamplingAdjustedAt = 0.0;
        double nominalAngleAdjustedAt = 0.0;
        double tolerancedSurfaceAdjustedAt = 0.0;
        double tolerancedSamplingAdjustedAt = 0.0;
        double tolerancedAngleStepAdjustedAt = 0.0;

        AxisUnits units;
    };

    using app::models::MaxPVResult;

    class SurfaceMapService {
        public:
            SurfaceMapService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(app::services::OperationMonitorService* monitor);

            void startCalculation(int surface, int sampling, double angle, app::models::TaskSource source);
            void cancelCalculation();
            std::function<void()> onNominalCalculationComplete;

            void startMapCalculation(int surface, int sampling, double angleStepDeg);
            void cancelMapCalculation();
            /// Fired on the UI thread when the last section of the map arrives,
            /// so the GUI can report completion without polling.
            std::function<void()> onMapCalculationComplete;

            /// Fired when GetRefresh reported a model Zemax cannot update; the
            /// calculation is not started (the GUI shows a modal dialog).
            std::function<void(const std::string& reason)> onUpdateFailed;

            /// Cancels the task owning the given UI-monitor task id (map run or
            /// the nominal reference profile). Returns false when nothing
            /// matches. See SurfaceProfileService::cancelTask for rationale.
            bool cancelTask(uint64_t taskId);

            void clearData();

            /// Called when a connection slot is torn down. Cancels any calculation
            /// bound to that slot so its resources are released safely.
            void onClientDisconnected(int index);

            bool hasData() const { return !m_profiles.empty() && m_totalAngles > 0 && static_cast<int>(m_profiles.size()) >= m_totalAngles; }
            const std::vector<app::models::SurfaceData>& getProfiles() const { return m_profiles; }
            const std::optional<MaxPVResult>& getMaxPVResult() const { return m_maxPVResult; }

            /// Cached data extents (unscaled sag units). The 3D renderers used to
            /// walk every point of every section each frame, which stalled the UI
            /// on precise maps; the extents are now recomputed only when the data
            /// changes (map completion, nominal load, clear).
            struct DataRanges {
                bool sagValid = false;
                float sagMin = 0.0f;
                float sagMax = 0.0f;
                bool devValid = false;
                float devMin = 0.0f;
                float devMax = 0.0f;
            };
            [[nodiscard]] const DataRanges& dataRanges() const { return m_ranges; }
            std::optional<MaxPVResult> findMaxPVSection() const;
            bool calculateDeviations();

            bool m_showTolerancedSurfaceMap{false};
            bool m_showDeviationSurfaceMap{false};
            bool m_showWorstSectionProfile{false};
            bool m_showWorstSectionDeviation{false};
            app::models::SurfaceData m_worstProfileData;

            MapWindowState m_windowState;
            app::models::SurfaceData m_nominalSurfaceData;

            int getTargetSampling() const { return m_targetSampling; }
            int getTotalAngles() const { return m_totalAngles; }
            double getAngleStepDeg() const { return m_angleStepDeg; }
            int getTotalDdeRequests() const { return m_totalDdeRequests; }
            int getNominalCalcSlot() const { return m_nominalCalcSlot; }
            int getMapCalcSlot() const { return m_mapCalcSlot; }

        protected:
            void startNextProfile();
            /// Runs the map after a successful lens refresh: clears the previous
            /// data, adopts the task registered for the refresh phase and starts
            /// the section loop.
            void beginMapRun(const app::compute::MapPlan& plan, int surface, uint64_t taskId);
            /// Recomputes the cached sag/deviation extents from the current data.
            void recomputeRanges();
            /// Client of the given slot, or nullptr when the slot is gone.
            ZemaxDDE::ZemaxDDEClient* resolveClient(int slot) const;
            /// Drops a queued GetRefresh and invalidates its callback.
            void abortPendingRefresh(bool nominal);
            /// Fails the task created for the refresh phase (not yet adopted).
            void cancelPendingRefreshTask(bool nominal);

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            app::services::OperationMonitorService* m_uiOpMonitor{nullptr};
            app::compute::SurfaceProfile m_nominalCalculator;
            app::compute::SurfaceProfile m_mapCalculator;
            app::compute::SurfaceMap m_mapEngine;

            std::vector<app::models::SurfaceData> m_profiles;
            std::optional<MaxPVResult> m_maxPVResult;
            DataRanges m_ranges;

            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_angleStepDeg = 0.0;
            int m_totalAngles = 0;
            int m_currentAngleIndex = 0;
            double m_centerSagRef = 0.0;
            int m_totalDdeRequests{0};
            uint64_t m_mapTaskId{0};
            int m_nominalCalcSlot{-1};
            int m_mapCalcSlot{-1};
            /// Invalidate pending refresh callbacks (cancel/restart/disconnect).
            uint64_t m_nominalRefreshToken{0};
            uint64_t m_mapRefreshToken{0};
            /// Tasks created for the refresh phase; adopted on success or failed
            /// on GetRefresh "-1".
            uint64_t m_nominalRefreshTaskId{0};
            uint64_t m_mapRefreshTaskId{0};
            std::chrono::steady_clock::time_point m_calcStartTime;
    };
}
