#pragma once

// Forward declaration for SagAnalysisService (not yet wired)
class SagAnalysisService;

// SagAnalysisController (Stage 4 consolidation)
class SagAnalysisController {
public:
    SagAnalysisController();
    void render();
private:
    // Future: SagAnalysisService* m_sag;
};
