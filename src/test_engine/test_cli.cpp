#include "test_engine/test_cli.h"

#include <cstring>
#include <string>

namespace app::testing {

TestCliOptions parseTestCliOptions(int argc, char** argv) {
    TestCliOptions options;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (std::strcmp(arg, "--test-engine") == 0) {
            options.showEngineUi = true;
        } else if (std::strcmp(arg, "--run-tests") == 0 && i + 1 < argc) {
            options.runTests = true;
            options.filter = argv[++i];
            if (options.filter.empty()) {
                options.filter = "all";
            }
        } else if (std::strcmp(arg, "--test-results") == 0 && i + 1 < argc) {
            options.resultsFile = argv[++i];
        } else if (std::strcmp(arg, "--test-verbose") == 0) {
            options.verbose = true;
        }
    }

    return options;
}

} // namespace app::testing
