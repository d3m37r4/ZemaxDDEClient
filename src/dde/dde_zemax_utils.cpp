#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    /*
    * Splits the input string into individual "tokens" (meaningful parts).
    * Tokens are delimited by commas, spaces, newlines, or carriage returns, unless they are enclosed within double quotes.
    * Returns a vector of strings, where each string is a token.
    */
    std::vector<std::string> tokenize(const std::string& bufferStr) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;

        for (char c : bufferStr) {
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if ((c == ',' || c == '\n' || c == '\r' || c == ' ') && !inQuotes) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }

        return tokens;
    }
}
