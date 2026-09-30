#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

#include "KMPAlgorithm.hpp"
#include "NaiveSearch.hpp"
#include "CountedSearch.hpp"
#include "InputGenerators.hpp"

// ===========================================================================
// Part 1: correctness tests (run before any timing)
// ===========================================================================

// Trusted reference: count all (possibly overlapping) matches with std::string::find.
int referenceSearch(const std::string& text, const std::string& pattern) {
    if (pattern.empty()) return 0;
    int count = 0;
    size_t pos = text.find(pattern);
    while (pos != std::string::npos) {
        count++;
        pos = text.find(pattern, pos + 1);
    }
    return count;
}

// Runs one input through every implementation (timed and counted versions).
// Also checks KMP's theoretical bounds: at most 2m comparisons to build the
// LPS array and at most 2n comparisons to scan the text.
bool checkCase(const std::string& text, const std::string& pattern, int expected) {
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    ComparisonCounts naiveCounts, kmpCounts;
    int ref = referenceSearch(text, pattern);
    int naive = naiveSearch(text, pattern);
    int kmp = kmpSearch(text, pattern);
    int naiveC = naiveSearchCounted(text, pattern, naiveCounts);
    int kmpC = kmpSearchCounted(text, pattern, kmpCounts);

    bool countsOk = (ref == expected && naive == expected && kmp == expected
        && naiveC == expected && kmpC == expected);
    bool boundsOk = (kmpCounts.preprocessing <= 2LL * m && kmpCounts.search <= 2LL * n);

    if (!countsOk || !boundsOk) {
        std::cout << "FAIL: text=\"" << text << "\" pattern=\"" << pattern << "\""
            << " expected=" << expected << " ref=" << ref
            << " naive=" << naive << " kmp=" << kmp
            << " naiveCounted=" << naiveC << " kmpCounted=" << kmpC
            << " kmpPreprocessComparisons=" << kmpCounts.preprocessing
            << " kmpSearchComparisons=" << kmpCounts.search << "\n";
        return false;
    }
    return true;
}

bool runCorrectnessTests() {
    int failures = 0;

    // Hand-picked cases, including edge cases.
    if (!checkCase("abababab", "aba", 3)) failures++;           // overlapping matches
    if (!checkCase("aaaaa", "aa", 4)) failures++;               // heavy overlap
    if (!checkCase("abcabc", "abcd", 0)) failures++;            // no match
    if (!checkCase("aabaaabaaab", "aabaaab", 2)) failures++;    // uses LPS fallback
    if (!checkCase("ab", "abc", 0)) failures++;                 // pattern longer than text
    if (!checkCase("abc", "abc", 1)) failures++;                // pattern == text
    if (!checkCase("", "abc", 0)) failures++;                   // empty text
    if (!checkCase("abc", "", 0)) failures++;                   // empty pattern

    // The LPS array itself, checked against hand-traced answers.
    struct LpsCase { std::string pattern; std::vector<int> expected; };
    std::vector<LpsCase> lpsCases = {
        {"aabaaab", {0, 1, 0, 1, 2, 2, 3}},
        {"abab",    {0, 0, 1, 2}},
        {"aaaa",    {0, 1, 2, 3}},
        {"abcd",    {0, 0, 0, 0}},
    };
    for (const LpsCase& c : lpsCases) {
        if (buildLPSArray(c.pattern) != c.expected) {
            std::cout << "FAIL: wrong LPS array for \"" << c.pattern << "\"\n";
            failures++;
        }
    }

    // Randomised cases: small alphabets give lots of partial matches,
    // which stress the fallback logic.
    std::mt19937 rng(12345);
    int randomTests = 0;
    for (int sigma : {1, 2, 3, 4, 26}) {
        for (int t = 0; t < 2000; t++) {
            int n = static_cast<int>(rng() % 100);
            int m = 1 + static_cast<int>(rng() % 10);
            std::string text = randomString(n, sigma, rng);
            std::string pattern = randomString(m, sigma, rng);
            if (!checkCase(text, pattern, referenceSearch(text, pattern))) failures++;
            randomTests++;
        }
    }

    std::cout << "Correctness: 8 fixed + 4 LPS + " << randomTests << " random tests, "
        << failures << " failures\n";
    return failures == 0;
}

// ===========================================================================
// Part 2: measurement
// ===========================================================================

// Stops the compiler from deleting searches whose result is never used.
volatile long long sink = 0;

// Times `run()` `repeats` times (after one warm-up) and returns the median in nanoseconds.
// `run` is any callable that returns an int (a lambda wrapping the search).
template <typename Fn>
long long medianTimeNs(Fn run, int repeats) {
    std::vector<long long> times;

    sink = sink + run();  // warm-up, not recorded

    for (int r = 0; r < repeats; r++) {
        auto start = std::chrono::steady_clock::now();
        int result = run();
        auto end = std::chrono::steady_clock::now();

        sink = sink + result;
        times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}

// Everything the CSV needs to know about one benchmark input.
struct InputInfo {
    std::string experiment;  // which experiment this row belongs to
    std::string inputType;   // random / adversarial / english
    int sigma;               // alphabet size (0 = not applicable, e.g. English text)
    int trial;               // which pattern draw this is (0, 1, 2, ...)
};

// Measures both algorithms on one (text, pattern) and writes one CSV row per algorithm.
void benchmarkInput(std::ofstream& csv, const InputInfo& info,
    const std::string& text, const std::string& pattern, int repeats) {
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    // 1) Timings, using the uninstrumented versions.
    long long naiveNs = medianTimeNs([&] { return naiveSearch(text, pattern); }, repeats);
    long long kmpNs = medianTimeNs([&] { return kmpSearch(text, pattern); }, repeats);
    long long lpsNs = medianTimeNs([&] { return static_cast<int>(buildLPSArray(pattern).size()); }, repeats);

    // 2) Comparison counts, using the instrumented versions (run once: counts are deterministic).
    ComparisonCounts naiveCounts, kmpCounts;
    int naiveMatches = naiveSearchCounted(text, pattern, naiveCounts);
    int kmpMatches = kmpSearchCounted(text, pattern, kmpCounts);

    // 3) Safety net: timed and counted versions must agree with each other.
    int timedNaive = naiveSearch(text, pattern);
    int timedKmp = kmpSearch(text, pattern);
    if (naiveMatches != kmpMatches || naiveMatches != timedNaive || kmpMatches != timedKmp) {
        std::cout << "WARNING: algorithms disagree in " << info.experiment
            << " n=" << n << " m=" << m << "\n";
    }

    // 4) Write the rows. KMP's time_ns includes building the LPS array;
    //    preprocess_ns is the LPS construction on its own.
    csv << info.experiment << ",naive," << info.inputType << "," << info.sigma << ","
        << info.trial << "," << n << "," << m << "," << naiveNs << ",0,"
        << naiveCounts.search << ",0," << naiveMatches << "\n";
    csv << info.experiment << ",kmp," << info.inputType << "," << info.sigma << ","
        << info.trial << "," << n << "," << m << "," << kmpNs << "," << lpsNs << ","
        << kmpCounts.search << "," << kmpCounts.preprocessing << "," << kmpMatches << "\n";
}

// ===========================================================================
// Part 3: experiments
// ===========================================================================

void runExperiments(const std::string& csvPath, const std::string& englishPath) {
    std::ofstream csv(csvPath);
    if (!csv) {
        std::cout << "Could not open " << csvPath << " for writing\n";
        return;
    }
    csv << "experiment,algorithm,input_type,sigma,trial,n,m,time_ns,preprocess_ns,"
        "comparisons,preprocess_comparisons,matches\n";

    std::mt19937 rng(42);    // fixed seed: the same inputs on every run
    const int repeats = 11;  // timed runs per measurement (median is reported)
    const int trials = 5;    // different random patterns per setting

    // Experiment 1: grow the text, fixed pattern length.
    // Prediction: both algorithms scale linearly in n.
    std::cout << "Experiment 1: random text, varying n\n";
    for (int n : {100000, 200000, 500000, 1000000, 2000000, 5000000, 10000000}) {
        std::string text = randomString(n, 4, rng);
        for (int t = 0; t < trials; t++) {
            benchmarkInput(csv, { "vary_n", "random", 4, t }, text, patternFromText(text, 10, rng), repeats);
        }
    }

    // Experiment 2: naive's worst case (text aaa...a, pattern aa...ab), growing m.
    // Prediction: naive does (n - m + 1) * m comparisons and slows down linearly in m;
    // KMP stays at about 2n comparisons, independent of m.
    std::cout << "Experiment 2: adversarial text, varying m\n";
    {
        const int n = 1000000;
        std::string text = repeatedChar(n, 'a');
        for (int m : {2, 5, 10, 20, 50, 100, 200, 500, 1000}) {
            benchmarkInput(csv, { "adversarial_vary_m", "adversarial", 2, 0 }, text, adversarialPattern(m), repeats);
        }
    }

    // Experiment 3: the same m values on random text.
    // Prediction: naive's worst-case bound grows with m, but on random text it fails
    // after about sigma/(sigma-1) comparisons per position, so it should stay flat.
    std::cout << "Experiment 3: random text, varying m\n";
    {
        const int n = 1000000;
        std::string text = randomString(n, 4, rng);
        for (int m : {2, 5, 10, 20, 50, 100, 200, 500, 1000}) {
            for (int t = 0; t < trials; t++) {
                benchmarkInput(csv, { "random_vary_m", "random", 4, t }, text, patternFromText(text, m, rng), repeats);
            }
        }
    }

    // Experiment 4: alphabet size.
    // Prediction: naive's comparisons per character fall towards 1 as sigma grows.
    std::cout << "Experiment 4: random text, varying alphabet size\n";
    for (int sigma : {2, 3, 4, 8, 16, 26}) {
        const int n = 1000000;
        std::string text = randomString(n, sigma, rng);
        for (int t = 0; t < trials; t++) {
            benchmarkInput(csv, { "vary_sigma", "random", sigma, t }, text, patternFromText(text, 20, rng), repeats);
        }
    }

    // Experiment 5: natural English text (skipped if the file isn't there).
    std::string english = loadTextFile(englishPath);
    if (english.empty()) {
        std::cout << "Experiment 5 skipped: could not read " << englishPath << "\n";
    }
    else {
        std::cout << "Experiment 5: English text (" << english.length() << " chars), varying m\n";
        for (int m : {2, 5, 10, 20, 50, 100}) {
            for (int t = 0; t < trials; t++) {
                benchmarkInput(csv, { "english_vary_m", "english", 0, t }, english, patternFromText(english, m, rng), repeats);
            }
        }
    }

    std::cout << "Results written to " << csvPath << "\n";
}

int main() {
    if (!runCorrectnessTests()) {
        std::cout << "Correctness tests failed. Not running benchmarks.\n";
        return 1;
    }

    auto start = std::chrono::steady_clock::now();
    runExperiments("results.csv", "data/english.txt");
    auto end = std::chrono::steady_clock::now();

    std::cout << "Total benchmark time: "
        << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " s\n";
    std::cout << "(checksum " << sink << ")\n";
    return 0;
}