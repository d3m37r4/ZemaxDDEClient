#include "utils.h"

namespace ZemaxDDE {
    /*
    * Splits the input string into individual "tokens" (meaningful parts).
    * Tokens are delimited by commas, spaces, newlines, or carriage returns, unless they are enclosed within double quotes.
    * Returns a vector of strings, where each string is a token.
    */
    std::vector<std::string> tokenize(std::string_view bufferStr) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;

        for (size_t i = 0; i < bufferStr.size(); ++i) {
            char c = bufferStr[i];
            // Handle escaped quotes: if we see a backslash followed by a quote, skip the backslash and add the quote literally
            if (c == '\\' && i + 1 < bufferStr.size() && bufferStr[i + 1] == '"') {
                currentToken += '"';
                ++i; // skip the quote character
                continue;
            }
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
