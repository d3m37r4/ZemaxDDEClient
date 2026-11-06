#include "gui/layout_snapshot.h"

#include <nlohmann/json.hpp>

namespace gui::layout {

    std::string serializeSnapshot(const Snapshot& snapshot) {
        nlohmann::json j;
        j["imguiVersion"] = snapshot.imguiVersion;
        j["imguiIni"] = snapshot.imguiIni;
        j["windowsJson"] = snapshot.windowsJson;
        j["chromeJson"] = snapshot.chromeJson;
        return j.dump(4);
    }

    bool deserializeSnapshot(std::string_view text, Snapshot& out) {
        if (text.empty()) {
            return false;
        }
        try {
            const nlohmann::json j = nlohmann::json::parse(text.begin(), text.end());
            if (!j.is_object()) {
                return false;
            }
            Snapshot parsed;
            parsed.imguiVersion = j.value("imguiVersion", 0);
            parsed.imguiIni = j.value("imguiIni", std::string{});
            parsed.windowsJson = j.value("windowsJson", std::string{});
            parsed.chromeJson = j.value("chromeJson", std::string{});
            if (parsed.imguiVersion == 0) {
                return false;
            }
            out = std::move(parsed);
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

}
