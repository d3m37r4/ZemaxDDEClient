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
        m_pendingRequests = 3;

        m_client.sendRequest("GetName",
            [this](const std::string& buffer) {
                std::string name = cp1251_to_utf8(buffer.data(), buffer.size());
                while (!name.empty() && (name.back() == '\0' || name.back() == '\r' || name.back() == '\n' || name.back() == ' '))
                    name.pop_back();
                m_opticalSystem.lensName = name.empty() ? "unknown" : name;
                m_logger.addLog(std::format("[InitLoad] lensName = '{}'", name));
                checkCompletion();
            },
            [this](const std::string& err) {
                onError(std::format("GetName: {}", err));
            });

        m_client.sendRequest("GetFile",
            [this](const std::string& buffer) {
                std::string fileName = cp1251_to_utf8(buffer.data(), buffer.size());
                while (!fileName.empty() && (fileName.back() == '\0' || fileName.back() == '\r' || fileName.back() == '\n' || fileName.back() == ' '))
                    fileName.pop_back();
                m_opticalSystem.fileName = fileName;
                m_logger.addLog(std::format("[InitLoad] fileName = '{}'", fileName));
                checkCompletion();
            },
            [this](const std::string& err) {
                onError(std::format("GetFile: {}", err));
            });

        m_client.sendRequest("GetSystem",
            [this](const std::string& buffer) {
                auto tokens = tokenize(buffer);
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
                checkCompletion();
            },
            [this](const std::string& err) {
                onError(std::format("GetSystem: {}", err));
            });
    }

    void InitialDataLoadService::checkCompletion() {
        if (m_state == LoadState::LoadingSystem && --m_pendingRequests == 0) {
            m_state = LoadState::LoadingFields;
            m_pendingRequests = 0;
            m_logger.addLog("[InitLoad] System loaded, loading fields");

            m_client.sendRequest("GetField,0",
                [this](const std::string& buffer) {
                    auto tokens = tokenize(buffer);
                    constexpr int EXPECTED = 5;
                    if (tokens.size() < EXPECTED) {
                        onError(std::format("GetField,0: expected {} tokens, got {}", EXPECTED, tokens.size()));
                        return;
                    }
                    try {
                        enum { FIELD_TYPE, NUM_FIELDS, MAX_X_FIELD, MAX_Y_FIELD, NORMALIZATION_METHOD };
                        m_opticalSystem.fieldType = std::stoi(tokens[FIELD_TYPE]);
                        int numFields = std::stoi(tokens[NUM_FIELDS]);
                        if (numFields < MIN_FIELDS || numFields > MAX_FIELDS) {
                            onError(std::format("GetField,0: numFields {} out of range [{}, {}]", numFields, MIN_FIELDS, MAX_FIELDS));
                            return;
                        }
                        m_opticalSystem.numFields = numFields;
                        m_opticalSystem.maxXField = std::stod(tokens[MAX_X_FIELD]);
                        m_opticalSystem.maxYField = std::stod(tokens[MAX_Y_FIELD]);
                        m_opticalSystem.normalizationMethod = std::stoi(tokens[NORMALIZATION_METHOD]);
                        m_logger.addLog(std::format("[InitLoad] Fields: type={}, count={}", m_opticalSystem.fieldType, numFields));
                        loadFields(numFields);
                    } catch (const std::exception& e) {
                        onError(std::format("GetField,0: parse error: {}", e.what()));
                    }
                },
                [this](const std::string& err) {
                    onError(std::format("GetField,0: {}", err));
                });
        } else if (m_state == LoadState::LoadingFields && --m_pendingRequests == 0) {
            m_state = LoadState::LoadingWaves;
            m_pendingRequests = 0;
            m_logger.addLog("[InitLoad] Fields loaded, loading waves");

            m_client.sendRequest("GetWave,0",
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
                        loadWaves(numWaves);
                    } catch (const std::exception& e) {
                        onError(std::format("GetWave,0: parse error: {}", e.what()));
                    }
                },
                [this](const std::string& err) {
                    onError(std::format("GetWave,0: {}", err));
                });
        } else if (m_state == LoadState::LoadingWaves && --m_pendingRequests == 0) {
            m_state = LoadState::Completed;
            m_logger.addLog(std::format("[InitLoad] Data load completed: {} fields, {} waves", m_opticalSystem.numFields, m_opticalSystem.numWaves));
        }
    }

    void InitialDataLoadService::loadFields(int numFields) {
        m_pendingRequests = numFields;

        for (int i = MIN_FIELDS; i <= numFields; ++i) {
            m_client.sendRequest(std::format("GetField,{}", i),
                [this, i](const std::string& buffer) {
                    auto tokens = tokenize(buffer);
                    if (tokens.size() >= 2) {
                        try {
                            m_opticalSystem.xField[i] = std::stod(tokens[0]);
                            m_opticalSystem.yField[i] = std::stod(tokens[1]);
                        } catch (const std::exception& e) {
                            onError(std::format("GetField,{}: parse error: {}", i, e.what()));
                            return;
                        }
                    }
                    checkCompletion();
                },
                [this, i](const std::string& err) {
                    onError(std::format("GetField,{}: {}", i, err));
                });
        }
    }

    void InitialDataLoadService::loadWaves(int numWaves) {
        m_pendingRequests = numWaves;

        for (int i = MIN_WAVES; i <= numWaves; ++i) {
            m_client.sendRequest(std::format("GetWave,{}", i),
                [this, i](const std::string& buffer) {
                    auto tokens = tokenize(buffer);
                    if (tokens.size() >= 2) {
                        try {
                            m_opticalSystem.waveData[i].value  = std::stod(tokens[0]);
                            m_opticalSystem.waveData[i].weight = std::stod(tokens[1]);
                        } catch (const std::exception& e) {
                            onError(std::format("GetWave,{}: parse error: {}", i, e.what()));
                            return;
                        }
                    }
                    checkCompletion();
                },
                [this, i](const std::string& err) {
                    onError(std::format("GetWave,{}: {}", i, err));
                });
        }
    }

    void InitialDataLoadService::onError(const std::string& msg) {
        m_state = LoadState::Failed;
        m_error = msg;
        m_logger.addLog(std::format("[InitLoad] Error: {}", msg));
    }

}
