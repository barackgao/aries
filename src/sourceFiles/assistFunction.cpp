//
// Created by 高炜 on 16/12/11.
//

#include "../headFiles/assistFunction.h"

void utilGetTokens(const std::string& str, const std::string& delimiter, std::vector<std::string>& tokens) {
    size_t start, end = 0;
    while (end < str.size()) {
        start = end;
        while (start < str.size() && (delimiter.find(str[start]) != std::string::npos)) {
            start++;
            // skip initial empty space
        }
        end = start;
        while (end < str.size() && (delimiter.find(str[end]) == std::string::npos)) {
            end++;
            //skip end of few words
        }
        if (end-start != 0) {  // ignore zero string.
            tokens.push_back(std::string(str, start, end-start));
        }
    }
}