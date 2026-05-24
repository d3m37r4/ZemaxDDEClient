#include <format>
#include <string>

#include "initial_data_load_service.h"
#include "client.h"
#include "constants.h"
#include "utils.h"
#include "logger/logger.h"

namespace ZemaxDDE {

    InitialDataLoadService::InitialDataLoadService(ZemaxDDEClient& client, OpticalSystemData& opticalSystem, Logger& logger)
        : m_client(client)
        , m_opticalSystem(opticalSystem)
        , m_logger(logger)
    {
    }

    void InitialDataLoadService::start() {
        if (m_state != LoadState::Idle) return;
        m_state = LoadState::LoadingSystem;
        m_error.clear();
        m_logger.addLog("[InitLoad] Starting initial data load");
        loadSystem();
    }

    void InitialDataLoadService::loadSystem() {
        m_client.submitRequest("GetName",
            [this](const std::string& buffer) {
                std::string name = cp1251_to_utf8(buffer.data(), buffer.size());
                while (!name.empty() && (name.back() == '\0' || name.back() == '\r' || name.back() == '\n' || name.back() == ' '))
                    name.pop_back();
                m_opticalSystem.lensName = name.empty() ? "unknown" : name;
                m_logger.addLog(std::format("[InitLoad] lensName = '{}'", m_opticalSystem.lensName));

                m_client.submitRequest("GetFile",
                    [this](const std::string& buf) {
                        std::string fileName = cp1251_to_utf8(buf.data(), buf.size());
                        while (!fileName.empty() && (fileName.back() == '\0' || fileName.back() == '\r' || fileName.back() == '\n' || fileName.back() == ' '))
                            fileName.pop_back();
                        m_opticalSystem.fileName = fileName;
                        m_logger.addLog(std::format("[InitLoad] fileName = '{}'", fileName));

                        m_client.submitRequest("GetSystem",
                            [this](const std::string& sysBuf) {
                                auto tokens = tokenize(sysBuf);
                                constexpr int EXPECTED = 9;
                                if (tokens.size() < EXPECTED) {
                                    onError(std::format("GetSystem: expected {} tokens, got {}", EXPECTED, tokens.size()));
                                    return;
                                }
                                try {
                                    enum { NUM_SURFS, UNIT_CODE, STOP_SURF, NON_AXIAL_FLAG, RAY_AIMING_TYPE, ADJUST_INDEX, TEMP, PRESSURE, GLOBAL_REF_SURF };
                                    m_opticalSystem.numSurfs      = std::stoi(tokens[NUM_SURFS]);
                                    m_opticalSystem.units         = std::stoi(tokens[UNIT_CODE]);
                                    m_opticalSystem.stopSurf      = std::stoi(tokens[STOP_SURF]);
                                    m_opticalSystem.nonAxialFlag  = std::stoi(tokens[NON_AXIAL_FLAG]);
                                    m_opticalSystem.rayAimingType = std::stoi(tokens[RAY_AIMING_TYPE]);
                                    m_opticalSystem.adjustIndex   = std::stoi(tokens[ADJUST_INDEX]);
                                    m_opticalSystem.temp          = std::stod(tokens[TEMP]);
                                    m_opticalSystem.pressure      = std::stod(tokens[PRESSURE]);
                                    m_opticalSystem.globalRefSurf = std::stoi(tokens[GLOBAL_REF_SURF]);
                                    m_logger.addLog(std::format("[InitLoad] System: {} surfaces, unit={}, stop={}", m_opticalSystem.numSurfs, m_opticalSystem.units, m_opticalSystem.stopSurf));
                                } catch (const std::exception& e) {
                                    onError(std::format("GetSystem: parse error: {}", e.what()));
                                    return;
                                }

                                m_state = LoadState::LoadingFields;
                                m_logger.addLog("[InitLoad] System loaded, loading fields");

                                m_client.submitRequest("GetField,0",
                                    [this](const std::string& fBuf) {
                                        auto ftokens = tokenize(fBuf);
                                        constexpr int FEXPECTED = 5;
                                        if (ftokens.size() < FEXPECTED) {
                                            onError(std::format("GetField,0: expected {} tokens, got {}", FEXPECTED, ftokens.size()));
                                            return;
                                        }
                                        try {
                                            enum { FIELD_TYPE, NUM_FIELDS, MAX_X_FIELD, MAX_Y_FIELD, NORMALIZATION_METHOD };
                                            m_opticalSystem.fieldType = std::stoi(ftokens[FIELD_TYPE]);
                                            int numFields = std::stoi(ftokens[NUM_FIELDS]);
                                            if (numFields < MIN_FIELDS || numFields > MAX_FIELDS) {
                                                onError(std::format("GetField,0: numFields {} out of range [{}, {}]", numFields, MIN_FIELDS, MAX_FIELDS));
                                                return;
                                            }
                                            m_opticalSystem.numFields = numFields;
                                            m_opticalSystem.maxXField = std::stod(ftokens[MAX_X_FIELD]);
                                            m_opticalSystem.maxYField = std::stod(ftokens[MAX_Y_FIELD]);
                                            m_opticalSystem.normalizationMethod = std::stoi(ftokens[NORMALIZATION_METHOD]);
                                            m_logger.addLog(std::format("[InitLoad] Fields: type={}, count={}", m_opticalSystem.fieldType, numFields));

                                            m_totalFields = numFields;
                                            m_currentField = MIN_FIELDS;
                                            loadNextField();
                                        } catch (const std::exception& e) {
                                            onError(std::format("GetField,0: parse error: {}", e.what()));
                                        }
                                    },
                                    [this](const std::string& err) {
                                        onError(std::format("GetField,0: {}", err));
                                    },
                                    2000, 1, "InitialDataLoad");
                            },
                            [this](const std::string& err) {
                                onError(std::format("GetSystem: {}", err));
                            },
                            2000, 1, "InitialDataLoad");
                    },
                    [this](const std::string& err) {
                        onError(std::format("GetFile: {}", err));
                    },
                    2000, 1, "InitialDataLoad");
            },
            [this](const std::string& err) {
                onError(std::format("GetName: {}", err));
            },
            2000, 1, "InitialDataLoad");
    }

    void InitialDataLoadService::loadNextField() {
        if (m_currentField > m_totalFields) {
            m_state = LoadState::LoadingWaves;
            m_logger.addLog("[InitLoad] Fields loaded, loading waves");

            m_client.submitRequest("GetWave,0",
                [this](const std::string& buffer) {
                    auto tokens = tokenize(buffer);
                    constexpr int EXPECTED = 2;
                    if (tokens.size() < EXPECTED) {
                        onError(std::format("GetWave,0: expected {} tokens, got {}", EXPECTED, tokens.size()));
                        return;
                    }
                    try {
                        enum { PRIM_WAVE, NUM_WAVES };
                        m_opticalSystem.primWave = std::stoi(tokens[PRIM_WAVE]);
                        int numWaves = std::stoi(tokens[NUM_WAVES]);
                        if (numWaves < MIN_WAVES || numWaves > MAX_WAVES) {
                            onError(std::format("GetWave,0: numWaves {} out of range [{}, {}]", numWaves, MIN_WAVES, MAX_WAVES));
                            return;
                        }
                        m_opticalSystem.numWaves = numWaves;
                        m_logger.addLog(std::format("[InitLoad] Waves: primary={}, count={}", m_opticalSystem.primWave, numWaves));

                        m_totalWaves = numWaves;
                        m_currentWave = MIN_WAVES;
                        loadNextWave();
                    } catch (const std::exception& e) {
                        onError(std::format("GetWave,0: parse error: {}", e.what()));
                    }
                },
                [this](const std::string& err) {
                    onError(std::format("GetWave,0: {}", err));
                },
                2000, 1, "InitialDataLoad");
            return;
        }

        m_client.submitRequest(std::format("GetField,{}", m_currentField),
            [this](const std::string& buffer) {
                auto tokens = tokenize(buffer);
                if (tokens.size() >= 2) {
                    try {
                        m_opticalSystem.xField[m_currentField] = std::stod(tokens[0]);
                        m_opticalSystem.yField[m_currentField] = std::stod(tokens[1]);
                    } catch (const std::exception& e) {
                        onError(std::format("GetField,{}: parse error: {}", m_currentField, e.what()));
                        return;
                    }
                }
                m_currentField++;
                loadNextField();
            },
            [this](const std::string& err) {
                onError(std::format("GetField,{}: {}", m_currentField, err));
            },
            2000, 1, "InitialDataLoad");
    }

    void InitialDataLoadService::loadNextWave() {
        if (m_currentWave > m_totalWaves) {
            m_state = LoadState::Completed;
            m_logger.addLog(std::format("[InitLoad] Data load completed: {} fields, {} waves", m_opticalSystem.numFields, m_opticalSystem.numWaves));
            return;
        }

        m_client.submitRequest(std::format("GetWave,{}", m_currentWave),
            [this](const std::string& buffer) {
                auto tokens = tokenize(buffer);
                if (tokens.size() >= 2) {
                    try {
                        m_opticalSystem.waveData[m_currentWave].value  = std::stod(tokens[0]);
                        m_opticalSystem.waveData[m_currentWave].weight = std::stod(tokens[1]);
                    } catch (const std::exception& e) {
                        onError(std::format("GetWave,{}: parse error: {}", m_currentWave, e.what()));
                        return;
                    }
                }
                m_currentWave++;
                loadNextWave();
            },
            [this](const std::string& err) {
                onError(std::format("GetWave,{}: {}", m_currentWave, err));
            },
            2000, 1, "InitialDataLoad");
    }

    void InitialDataLoadService::onError(const std::string& msg) {
        m_state = LoadState::Failed;
        m_error = msg;
        m_logger.addLog(std::format("[InitLoad] Error: {}", msg));
    }

}
