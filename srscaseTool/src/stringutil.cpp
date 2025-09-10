#include "stringutil.h"

namespace string_util {

    std::vector<size_t> FindMatches(const std::wstring& content, const std::wstring& searchText) {
        std::vector<size_t> matches;

        if (searchText.empty() || content.empty() || searchText.size() > content.size()) {
            return matches;
        }

        // Pre-process normalization only if needed
        if (content.find(L"\r\n") != std::wstring::npos) {
            std::wstring normalizedContent;
            normalizedContent.reserve(content.size()); // Reserve to avoid reallocations

            // Single-pass normalization with better performance
            for (size_t i = 0; i < content.size(); ++i) {
                if (i + 1 < content.size() && content[i] == L'\r' && content[i + 1] == L'\n') {
                    normalizedContent += L'\n';
                    ++i; // Skip the next character
                }
                else {
                    normalizedContent += content[i];
                }
            }

            // Use Boyer-Moore or efficient search algorithm
            const size_t patternLength = searchText.size();
            size_t pos = 0;

            while ((pos = normalizedContent.find(searchText, pos)) != std::wstring::npos) {
                matches.push_back(pos);
                pos += patternLength; // Jump by pattern length for non-overlapping
                // For overlapping matches, use: pos += 1;
            }
        }
        else {
            // No normalization needed - search directly
            const size_t patternLength = searchText.size();
            size_t pos = 0;

            while ((pos = content.find(searchText, pos)) != std::wstring::npos) {
                matches.push_back(pos);
                pos += patternLength; // Jump by pattern length for non-overlapping
                // For overlapping matches, use: pos += 1;
            }
        }

        return matches;
    }

  }
