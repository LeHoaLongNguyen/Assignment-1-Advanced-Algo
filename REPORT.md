# KMP vs Naive: Empirical Study Report

**Le Hoa Long Nguyen, 26117008, 41052** · Assignment 1, Track A

## (a) What I built

I implemented the Knuth–Morris–Pratt (KMP) string-matching algorithm in C++17 and compared it empirically against the naive (brute-force) algorithm. I chose to do this algorithm as i was particularly interested in a string-matching algorithm and i thought it was be easier to understand as opposed to some of the other options.

**The algorithm.** KMP runs in two phases. It first builds the LPS array (longest proper prefix that is also a suffix) for the pattern in O(m). It then scans the text once in O(n), using the LPS array to decide how far to fall back after a mismatch, so it never re-reads a text character. For each position i, lps[i] stores the length of the biggest prefix of the pattern that is also the suffix up to i. For `aabaaab` the array is 0, 1, 0, 1, 2, 2, 3; for example, lps[6] = 3 because the whole pattern both starts and ends with aab. During the search, when a mismatch happens after j characters have matched, only j falls back, to lps[j − 1], while i keeps moving forward and never backtracks. This means KMP keeps the part of the match that could still start a new match, instead of re-reading text it has already seen. My understanding of the algorithm came mainly from two youtube videos (Adbul Bari;ByteQuest) with Claude explanations.

**Key design decisions**

| Decision            | Choice                                                | Reason                                                                                            |
| ------------------- | ----------------------------------------------------- | ------------------------------------------------------------------------------------------------- |
| Language            | C++17, MSVC, Release x64 (/O2)                        | Compiled code, so timings reflect the algorithms rather than interpreter overhead                 |
| Output              | Count of all matches, including overlapping ones      | Every run scans the whole text, so timings aren't affected by where the first match happens to be |
| Structure           | `buildLPSArray` and `kmpSearch` as separate functions | Preprocessing can be timed and counted separately from searching                                  |
| Comparison counting | Separate instrumented copies (`CountedSearch`)        | A counter inside the timed functions would slow them down and distort timings                     |
| Empty pattern       | Returns 0                                             | Avoids `lps[j - 1]` reading out of bounds                                                         |

**Simplifications**

My implementation follows the standard LPS version of KMP rather than the optimised table in the original paper. It returns the number of matches instead of their positions, so that timings measure searching rather than storing results. As an extension for this study, I also wrote instrumented copies of both algorithms that count character comparisons, with preprocessing and search counted separately. Empty pattern returns 0. Mathematically an empty pattern "matches" at every position; you chose 0 to avoid reading lps[-1].

**Code map**

| File                       | Contents                                                              |
| -------------------------- | --------------------------------------------------------------------- |
| `KMPAlgorithm.hpp/.cpp`    | `buildLPSArray` and `kmpSearch`                                       |
| `NaiveSearch.hpp/.cpp`     | `naiveSearch`, the baseline                                           |
| `CountedSearch.hpp/.cpp`   | Copies of both algorithms that count character comparisons            |
| `InputGenerators.hpp/.cpp` | Random, adversarial and file-based text generation                    |
| `main.cpp`                 | Correctness tests, then the five experiments; writes `results.csv`    |
| `plot_results.py`          | Reads `results.csv`; writes the figures and `summary.csv` to `plots/` |

## (b) Empirical study

### What I compared

I compared KMP against the naive algorithm, which checks every alignment of the pattern from left to right and stops at the first mismatch. Naive is the algorithm KMP was designed to improve on, so it gives the clearest test of whether KMP's better bound matters in practice.

Each algorithm was measured in two ways:

- **Character comparisons**, a machine-independent count that should match the theory exactly.
- **Wall-clock time**, which shows how the algorithm behaves on a real CPU and compiler.

Comparing the two measures is the core of the study: where they disagree, practice has diverged from theory.

### Theory and hypotheses

| Measure                      | Naive                                                               | KMP                                                        |
| ---------------------------- | ------------------------------------------------------------------- | ---------------------------------------------------------- |
| Worst case                   | O(nm): (n − m + 1) × m comparisons on text `aaa…a`, pattern `aa…ab` | O(n + m): at most 2n search + 2m preprocessing comparisons |
| Random text, alphabet size σ | About σ/(σ − 1) comparisons per text character                      | Between 1 and 2 per character                              |
| Preprocessing                | None                                                                | O(m) to build the LPS array                                |


- **H1 (scaling in n):** both algorithms' running times grow linearly with n.
- **H2 (worst case):** on the adversarial input, naive's time grows linearly with m while KMP's stays constant.
- **H3 (typical case):** on random text, naive's cost does not grow with m, because it fails after about σ/(σ − 1) comparisons per alignment.
- **H4 (alphabet size):** KMP's advantage in comparisons is largest for small σ and disappears as σ grows.
- **H5 (natural text):** English text behaves like a large alphabet, so naive matches or beats KMP.
- **H6 (my own prediction): KMP would outperform the naive algorithm in all cases.

### Method

**Correctness first.** Before any timing, the program checks that naive, KMP and both counted copies agree with a reference built on `std::string::find`. The tests cover 8 hand-picked edge cases, 4 hand-traced LPS arrays, and 10,000 random inputs over alphabets of size 1, 2, 3, 4 and 26. On every input they also check KMP's bounds: at most 2m preprocessing comparisons and 2n search comparisons. The benchmarks run only if every test passes.

**Experiments.** Each experiment changes one variable and holds the rest fixed.

| #   | Input                                                                                                              | Varied                    | Fixed                 |
| --- | ------------------------------------------------------------------------------------------------------------------ | ------------------------- | --------------------- |
| 1   | Random text, σ = 4                                                                                                 | n = 100,000 to 10,000,000 | m = 10                |
| 2   | Adversarial: text `aaa…a`, pattern `aa…ab`                                                                         | m = 2 to 1,000            | n = 1,000,000         |
| 3   | Random text, σ = 4                                                                                                 | m = 2 to 1,000            | n = 1,000,000         |
| 4   | Random text                                                                                                        | σ = 2 to 26               | n = 1,000,000, m = 20 |
| 5   | English text: Alice's Adventures in Wonderland (Lewis Carroll), Millennium Fulcrum Edition 3.0, 152,138 characters | m = 2 to 100              | the book              |

**Data generation.**

- Random text draws each character uniformly from the first σ letters (`a`, `b`, …) using `std::mt19937`.
- Patterns for random and English text are copied from a random position in the text, so each has at least one match.
- The seed is fixed at 42, so every run generates identical inputs.
- Each random setting is tested with 5 different patterns, to avoid conclusions based on one unusual pattern.

**Measurement.**

- Time: `std::chrono::steady_clock`, one warm-up run, then the median of 11 timed runs. Input generation happens outside the timed region.
- KMP's total time includes building the LPS array. The LPS construction is also timed on its own.
- A running checksum of results is printed at the end, so the compiler cannot remove searches whose result is unused.
- Comparisons: counted by the instrumented copies, run once per input, since counts are deterministic.
- Across the 5 patterns, times are summarised by the median and comparisons by the mean.

**Machine.**

| Component        | Value                                             |
| ---------------- | ------------------------------------------------- |
| CPU              | AMD Ryzen 7 4800H with Radeon Graphics            |
| RAM              | 16.0 GB DDR4                                      |
| Operating system | Windows 11                                        |
| Compiler         | MSVC 14.39 (Visual Studio 2022), Release x64, /O2 |

### Results

In running time, naive was faster than KMP on most realistic inputs, typically by about 5–10%, even though it made more character comparisons. KMP was faster on every adversarial setting, by up to 372× at m = 1,000. A few other settings went KMP's way (binary text, n = 1,000,000 in experiment 1, and English text at m = 5), but by margins small or inconsistent enough to be timing noise. In comparisons, KMP never made more than naive, apart from a single extra comparison at m = 2 on the adversarial input.

#### Experiment 1: growing text length (H1)

![Figure 1: random text, growing n](plots/fig1_vary_n.png)

| n          | Naive    | KMP      |
| ---------- | -------- | -------- |
| 100,000    | 0.304 ms | 0.332 ms |
| 1,000,000  | 4.15 ms  | 4.00 ms  |
| 10,000,000 | 31.0 ms  | 33.1 ms  |

H1 held: going from 100,000 to 10,000,000 characters (100× more text) took about 100× longer for both algorithms. Naive was faster at 6 of the 7 sizes, by 7–14%. The exception, n = 1,000,000, where KMP was 4% faster, is most likely timing noise, since it breaks the pattern of every other size.

#### Experiment 2: adversarial input (H2)

![Figure 2: adversarial input, growing m](plots/fig2_adversarial.png)

| m     | Naive   | KMP     | Naive comparisons | KMP comparisons |
| ----- | ------- | ------- | ----------------- | --------------- |
| 2     | 2.38 ms | 2.03 ms | 1,999,998         | 1,999,999       |
| 10    | 7.87 ms | 2.02 ms | 9,999,910         | 1,999,991       |
| 100   | 78.9 ms | 1.93 ms | 99,990,100        | 1,999,901       |
| 1,000 | 726 ms  | 1.95 ms | 999,001,000       | 1,999,001       |

H2 held. Naive's comparisons equal (n − m + 1) × m exactly, and its time grew in proportion to m, from 2.38 ms to 726 ms. KMP stayed just under 2n comparisons and about 2 ms for every m, making it 372× faster at m = 1,000.

#### Experiment 3: random text, growing pattern length (H3)

![Figure 3: random text, growing m](plots/fig3_random_vary_m.png)

H3 held. Naive stayed between 3.28 and 3.43 ms and at 1.333 comparisons per character for every m from 5 to 1,000, matching σ/(σ − 1) = 4/3. KMP made fewer comparisons (about 1.25 per character) but took 3.55–3.72 ms, 5–9% slower at every m. At m = 2 naive made 1.25 comparisons per character, which matches the exact expectation 1 + 1/σ for a 2-character pattern.

#### Experiment 4: alphabet size (H4)

![Figure 4: random text, growing alphabet size](plots/fig4_vary_sigma.png)

| σ   | Naive comparisons/char | KMP comparisons/char | Faster          |
| --- | ---------------------- | -------------------- | --------------- |
| 2   | 2.00                   | 1.39                 | KMP, about 1.5% |
| 4   | 1.33                   | 1.25                 | Naive, about 4% |
| 8   | 1.14                   | 1.12                 | Naive, about 2% |
| 26  | 1.04                   | 1.04                 | Tie (within 1%) |

H4 held. Naive's comparisons follow σ/(σ − 1) at every σ, and the gap between the algorithms closes as σ grows. σ = 2 is the only random-text setting where KMP was faster, in both of my runs (by about 3% in an earlier run and 1.5% in this one). The advantage is consistent but small.

#### Experiment 5: English text (H5)

![Figure 5: English text, growing m](plots/fig5_english.png)

| m   | Naive    | KMP      | Naive comparisons/char | KMP comparisons/char | Faster           |
| --- | -------- | -------- | ---------------------- | -------------------- | ---------------- |
| 2   | 0.203 ms | 0.239 ms | 1.072                  | 1.063                | Naive, about 15% |
| 5   | 0.216 ms | 0.176 ms | 1.041                  | 1.033                | KMP, about 23%   |
| 10  | 0.208 ms | 0.226 ms | 1.046                  | 1.038                | Naive, about 9%  |
| 20  | 0.188 ms | 0.191 ms | 1.065                  | 1.059                | Naive, about 1%  |
| 50  | 0.183 ms | 0.185 ms | 1.041                  | 1.032                | Naive, about 1%  |
| 100 | 0.222 ms | 0.229 ms | 1.082                  | 1.074                | Naive, about 3%  |

H5 held for comparisons. Both algorithms made only 1.03–1.08 comparisons per character, less than 1% apart, so English behaves like random text over a large alphabet: almost every alignment fails on the first character. Naive was faster in 5 of the 6 settings, but the timings are less reliable than in the other experiments. The text is only 152,138 characters, so each search takes about 0.2 ms, and the results jump around without a clear trend as m grows (KMP's 23% win at m = 5 is the clearest example).

#### Summary across experiments

![Figure 6: speed-up of KMP over naive](plots/fig6_speedup.png)

![Figure 7: share of KMP's time spent building the LPS array](plots/fig7_preprocessing_share.png)

Building the LPS array took under 0.08% of KMP's total time even at m = 1,000, with n = 1,000,000.

### Analysis: why the results came out this way

The results can be explained by splitting running time into two factors: time = number of comparisons × cost per comparison. Theory predicts the first factor exactly, but says nothing about the second.

| Setting                 | Naive comparisons/char | KMP comparisons/char | Naive ns/comparison | KMP ns/comparison |
| ----------------------- | ---------------------- | -------------------- | ------------------- | ----------------- |
| Random, σ = 4 (m = 100) | 1.333                  | 1.250                | 2.55                | 2.86              |
| Random, σ = 2           | 2.000                  | 1.391                | 2.54                | 3.59              |
| Adversarial, m = 1,000  | 999.0                  | 1.999                | 0.73                | 0.98              |

**Where theory and practice agree.** Comparison counts matched the formulas almost exactly: naive made exactly (n − m + 1) × m comparisons on the adversarial input, and 1.333 per character on random text with σ = 4, matching σ/(σ − 1). This is because a comparison count measures the same abstract operation the theory analyses, on inputs whose statistics are known exactly. It is deterministic and machine-independent. Running time also depends on how expensive each comparison is on a real CPU: loop overhead, memory reads, branch prediction and what the compiler does with the code. The comparison model treats all of these as free.

**Divergence 1: KMP does less work but runs slower on typical text.** On random text with σ = 4, KMP made about 6% fewer comparisons than naive (1.25 against 1.33 per character) but took 5–9% longer, because each of its comparisons cost about 12% more (2.86 ns against 2.55 ns). The likely reasons are in the structure of the two loops:

- In naive, each alignment is independent: whether position i matches doesn't affect where the next alignment starts, because i always increases by one. The CPU can start work on the next alignment before the current one has finished.
- In KMP, every iteration depends on the previous one. The next comparison uses j, which is only known once the previous comparison and, on a mismatch, the `lps[j - 1]` lookup have finished. This forms a chain the CPU has to follow one step at a time.
- KMP also chooses between three cases on every iteration based on the data, and a mismatch with j > 0 costs a whole extra iteration and a memory read without advancing i. Naive has no equivalent cost.

**Divergence 2: saving comparisons only pays off when the saving is large.** KMP is faster only when the comparisons it saves outweigh its higher cost per comparison. At σ = 4 it saves about 6% of comparisons but pays about 12% more per comparison, so it loses. At σ = 2 it saves about 30% but each comparison costs about 42% more (more mismatches trigger fallbacks), so the two nearly cancel, which matches the small 1.5% win. On the adversarial input, naive needs 999 comparisons per text character at m = 1,000 against KMP's 2, a saving of 99.8%. No difference in cost per comparison can make up for that, which is why KMP was 372× faster. Interestingly, naive's cost per comparison drops to 0.73 ns on the adversarial input: its inner loop repeats the same successful comparison hundreds of times in a row, which the CPU predicts well. Even so, it cannot overcome doing 500 times more work.

**Divergence 3: equal work gives no reliable winner.** At m = 2 on the adversarial input, both algorithms made exactly 2n comparisons. In an earlier run naive was about 10% faster there; in the final run KMP was about 17% faster. With the work held equal, the difference between them is within run-to-run variation. This shows that small timing differences at a single setting should not be over-interpreted, and it is why I used medians of repeated runs and several patterns.

**Compiler effect.** A preliminary run of the same code with g++ on Linux (run during development with AI assistance, not on my machine) showed naive about 20% faster than KMP on random text, compared with 5–9% under MSVC. The cost per comparison therefore depends on the compiler, not only the algorithm. Large asymptotic differences, like the 372× on adversarial input, hold regardless of compiler, but small constant-factor differences may not carry over to other platforms.

**Conclusion.** For typical text, such as English or random data, the naive algorithm was slightly faster than KMP in practice, because its simpler loop outweighs KMP's small saving in comparisons. KMP's real advantage is its guarantee: it never does more than 2n comparisons, so it cannot be slowed down by repetitive or adversarial input, where naive became hundreds of times slower. Because it never moves backwards in the text, it can also process a stream of data without storing it. I would use a simple or library search for ordinary text, and KMP when the input could be highly repetitive (such as DNA or log files) or controlled by an untrusted user.

### Limitations

- **Timer resolution.** The Windows clock ticks every 100 ns, and building the LPS array for short patterns takes less than one tick. Preprocessing times for small m are only upper bounds. Timing many LPS constructions in a loop and dividing would fix this.
- **One machine, one compiler.** All final results come from one CPU with MSVC. The g++ comparison suggests the size of the naive–KMP gap is compiler-dependent.
- **Timing noise.** Some individual measurements vary between patterns and runs. The median of 11 runs and 5 patterns reduces this but doesn't remove it. Differences of 1–2% should be treated as ties.
- **Input range.** Random text is uniformly distributed, which real data rarely is. Only one English text was tested.
- **Baseline choice.** Only naive was compared. A practical baseline such as Boyer–Moore–Horspool or `std::string::find` might change the conclusions.

## (c) What I learned

**What surprised me.**
What surprised me the most was that while the KMP algorithm was consistently doing fewer character comparisons than the naive approach on all tests that were performed, it still ran slower than the naive approach on most cases other than the worst case scenarios such as `aaaa....aa` or `aaaaaab`. My first guess was that the LPS preprocessing slowed KMP down, since naive starts searching straight away. My own Figure 7 showed that was wrong: building the table took under 0.08% of KMP's total time. The real difference was in the search loop itself, where each KMP comparison cost about 12% more than a naive one. This made my hypothesis incorrectly as it shows that theory and practicality don't always matter.

**What I initially got wrong.**
There were a few logic errors when i was first typing up the KMP search, one that really stuck with me was when i was comparing `text[i]` with `pattern[i]` instead of `pattern[j]` which resulted in a string subcript out of range crash. Luckily the use of ai was approved for this assignment and it could detect my mistake instantly.

**Something I understand now that I didn't before.** 
The most difficult part of the algorithm was understanding how the line len = lps[len - 1] works. I asked Claude to explain it, and the key idea is that when a mismatch happens, the current match of length len is finished, but part of it might still be the start of a new match. The longest part that could still work is the longest prefix of the pattern that is also a suffix of what has already matched, and that length is already stored in lps[len - 1]. So instead of starting over from 0, the loop falls back to that shorter match and tries again.

Tracing aabaaab made this concrete. At i = 5, the match aa could not be extended because a did not equal b. len fell back to lps[1] = 1, and with i still at 5, the a then matched, giving lps[5] = 2. If I had reset len to 0, the result would have been 1, which is wrong. This also showed me why i does not move on a fallback: the mismatch only rules out the current match, not the character itself, so the same character has to be tried again against the shorter match.

## (d) AI use

I used Claude (Anthropic) throughout this project as a tutor and as a code generator for the benchmarking infrastructure, i also used it to improve my grammar and sentence structure.

| Part                                                                                                 | Who wrote it          | How AI was involved                                                                                                                                                                    |
| ---------------------------------------------------------------------------------------------------- | --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Choosing the baseline and experiment design                                                          | Me with AI assistance | I decided to compare against naive as the baseline whilst Claude proposed the input families and the two measures (comparisons, time)                                                  |
| Visual Studio setup and file structure                                                               | Me                    | I set it up; Claude explained the steps and project settings                                                                                                                                                                                   |
| `buildLPSArray`                                                                                      | Me with AI assistance | Claude provided help to turn the theory into working code and created an interactive trace; I tested it on hand-traced patterns                                                        |
| `kmpSearch`                                                                                          | Me                    | Claude explained the approach and gave a trace. I wrote the code; Claude found two bugs in my first version (`pattern[i]` for `pattern[j]`, and the `else (j == 0);` semicolon)        |
| `naiveSearch`                                                                                        | Claude                | Written at my request, with a line-by-line explanation                                                                                                                                 |
| Benchmark harness, counted copies, input generators (`main.cpp`, `CountedSearch`, `InputGenerators`) | Claude                | Written by Claude and explained to me; I ran them on my machine                                                                                                                        |
| `plot_results.py`                                                                                    | Claude                | Written by Claude                                                                                                                                                                      |
| Verification                                                                                         | Claude                | Claude tested my code against a brute-force reference on 100,000 random inputs                                                                                                         |
| Interpreting results                                                                                 | Me, with AI help      | Claude explained what the CSV contained and summarised the findings; the analysis in section (b) was refined by Claude when trying to articulate my words after i reviewed the results |
| This report                                                                                          | Me                    | Claude produced this template with my measured numbers filled in. The analysis, section (c) and the final wording are mine                                                             |

AI was very helpful when creating the benchmarking harness for this project as it laid out and explained everything out nicely as i could tell if i tried to create this harness in c++ without the use of AI is would have been the hardest part of the assignment. That being said i extensively checked on what was being produced in the CSV file and checking if it was legitimate information. What i would do differently next time is probably change the programming language to python or c# as i have more experience with that.

## References

1. Knuth, D. E., Morris, J. H., and Pratt, V. R. (1977). Fast pattern matching in strings. _SIAM Journal on Computing_, 6(2), 323–350.
2. Abdul Bari, 9.1 Knuth-Morris-Pratt KMP String Matching Algorithm , online video, YouTube, viewed 25 September 2026, https://www.youtube.com/watch?v=V5-7GzOfADQ
3. ByteQuest, Knuth-Morris-Pratt Algorithm , online video, YouTube, viewed 25 septhember 2026, https://www.youtube.com/watch?v=q4_90fOoS-s
4. Carroll, L. 1865, Alice's Adventures in Wonderland, Millennium Fulcrum Edition 3.0, Project Gutenberg eBook #11.

## Appendix: reproducing the results

1. Open the solution in Visual Studio 2022 and select **Release** and **x64**.
2. Put the English text at `data/english.txt` in the project folder.
3. Run with Ctrl+F5. The program runs the correctness tests, then all five experiments, and writes `results.csv`.
4. Run `python plot_results.py` (requires pandas and matplotlib) to produce the figures and `summary.csv` in `plots/`.

The random seed is fixed, so the inputs are identical on every run. Times will differ on other machines.
