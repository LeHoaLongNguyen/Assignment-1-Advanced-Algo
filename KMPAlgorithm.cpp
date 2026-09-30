#include "KMPAlgorithm.hpp"

std::vector<int> buildLPSArray(const std::string& pattern) {
	int m = pattern.length();
	std::vector<int> LongestPrefixSuffixArray(m, 0);

	int len = 0;
	int i = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            LongestPrefixSuffixArray[i] = len;
            i++;
        }
        else if (len > 0) {
            len = LongestPrefixSuffixArray[len - 1];
        }
        else {
            LongestPrefixSuffixArray[i] = 0;
            i++;
        }
    }

    return LongestPrefixSuffixArray;
}

int kmpSearch(const std::string& text, const std::string& pattern) {
    int n = text.length();
    int m = pattern.length();

    if (m == 0) return 0;

    std::vector<int> lps = buildLPSArray(pattern);

    int i = 0;
    int j = 0;
    int count = 0;

    while (i < n) {
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

