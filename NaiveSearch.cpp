#include "NaiveSearch.hpp";

int naiveSearch(const std::string& text, const std::string& pattern) {
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0) return 0;

    int count = 0;

    for (int i = 0; i <= n - m; i++) {
        int j = 0;

        while (j < m && text[i + j] == pattern[j]) {
            j++;
        }

        if (j == m) {
            count++;
        }
    }

    return count;
}