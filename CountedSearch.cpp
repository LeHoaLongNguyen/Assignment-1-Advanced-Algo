#include "CountedSearch.hpp"

std::vector<int> buildLPSArrayCounted(const std::string& pattern, long long& comparisons) {
    int m = static_cast<int>(pattern.length());
    std::vector<int> lps(m, 0);

    int len = 0;
    int i = 1;

    while (i < m) {
        comparisons++;  // the pattern[i] == pattern[len] check below
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else if (len > 0) {
            len = lps[len - 1];
        }
        else {
            lps[i] = 0;
            i++;
        }
    }

    return lps;
}

int kmpSearchCounted(const std::string& text, const std::string& pattern, ComparisonCounts& counts) {
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0) return 0;

    std::vector<int> lps = buildLPSArrayCounted(pattern, counts.preprocessing);

    int i = 0;
    int j = 0;
    int count = 0;

    while (i < n) {
        counts.search++;  // the text[i] == pattern[j] check below
        if (text[i] == pattern[j]) {
            i++;
            j++;
            if (j == m) {
                count++;
                j = lps[j - 1];
            }
        }
        else if (j > 0) {
            j = lps[j - 1];
        }
        else {
            i++;
        }
    }

    return count;
}

int naiveSearchCounted(const std::string& text, const std::string& pattern, ComparisonCounts& counts) {
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0) return 0;

    int count = 0;

    for (int i = 0; i <= n - m; i++) {
        int j = 0;

        // Same logic as naiveSearch's while loop, split up so each
        // character comparison can be counted.
        while (j < m) {
            counts.search++;
            if (text[i + j] != pattern[j]) break;
            j++;
        }

        if (j == m) {
            count++;
        }
    }

    return count;
}