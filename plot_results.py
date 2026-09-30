"""
Reads results.csv (written by the C++ benchmark) and produces the figures
for the report in the plots/ folder, plus a summary table on the console.

Usage:  python plot_results.py            (reads results.csv, writes plots/)
        python plot_results.py other.csv  (reads a different results file)

Requires: pandas, matplotlib   (pip install pandas matplotlib)
"""

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

CSV_PATH = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("results.csv")
OUT_DIR = Path("plots")

# One fixed colour per algorithm, used in every figure.
COLORS = {"naive": "#2a78d6", "kmp": "#eb6834"}
LABELS = {"naive": "Naive", "kmp": "KMP"}
INK = "#52514e"      # axis labels and tick text
GRID = "#e4e3df"     # hairline grid

plt.rcParams.update({
    "font.size": 10,
    "axes.edgecolor": GRID,
    "axes.labelcolor": INK,
    "axes.titlesize": 11,
    "axes.titleweight": "normal",
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.color": GRID,
    "grid.linewidth": 0.6,
    "xtick.color": INK,
    "ytick.color": INK,
    "legend.frameon": False,
    "savefig.dpi": 200,
    "savefig.bbox": "tight",
})


def load() -> pd.DataFrame:
    df = pd.read_csv(CSV_PATH)
    df["time_ms"] = df["time_ns"] / 1e6
    # Comparisons per text character: the machine-independent cost measure.
    df["comparisons_per_char"] = df["comparisons"] / df["n"]
    return df


def summarise(df: pd.DataFrame, experiment: str, x: str) -> pd.DataFrame:
    """Combine the trials for each setting: median time, mean comparisons."""
    sub = df[df["experiment"] == experiment]
    aggregations = dict(time_ms=("time_ms", "median"),
                        preprocess_ms=("preprocess_ns", lambda s: s.median() / 1e6),
                        comparisons_per_char=("comparisons_per_char", "mean"))
    if x != "n":
        aggregations["n"] = ("n", "first")  # n is fixed within these experiments
    return (sub.groupby(["algorithm", x], as_index=False)
               .agg(**aggregations)
               .sort_values(x))


def plot_series(ax, data: pd.DataFrame, x: str, y: str) -> None:
    for alg in ["naive", "kmp"]:
        d = data[data["algorithm"] == alg]
        ax.plot(d[x], d[y], color=COLORS[alg], linewidth=2, marker="o",
                markersize=5, label=LABELS[alg])


def two_panel(title: str, data: pd.DataFrame, x: str, xlabel: str,
              logx: bool = False, logy_time: bool = False):
    """Left panel: wall-clock time. Right panel: comparisons per text character."""
    fig, (ax_t, ax_c) = plt.subplots(1, 2, figsize=(10, 3.8))
    fig.suptitle(title, x=0.01, ha="left", fontsize=12, color="#0b0b0b")

    plot_series(ax_t, data, x, "time_ms")
    ax_t.set_title("Wall-clock time (median of runs)", loc="left", color=INK)
    ax_t.set_xlabel(xlabel)
    ax_t.set_ylabel("Time (ms)")
    ax_t.set_ylim(bottom=0) if not logy_time else ax_t.set_yscale("log")

    plot_series(ax_c, data, x, "comparisons_per_char")
    ax_c.set_title("Character comparisons per text character", loc="left", color=INK)
    ax_c.set_xlabel(xlabel)
    ax_c.set_ylabel("Comparisons / n")

    for ax in (ax_t, ax_c):
        if logx:
            ax.set_xscale("log")
    return fig, ax_t, ax_c


THEORY_STYLE = {"naive": "--", "kmp": ":"}  # dashed = naive prediction, dotted = KMP bound


def add_theory_line(ax, xs, ys, alg: str, label: str) -> None:
    """Theoretical prediction, drawn in dark ink ON TOP of the measured line so both stay visible."""
    ax.plot(xs, ys, color="#0b0b0b", linewidth=1.3, linestyle=THEORY_STYLE[alg],
            label=label, zorder=5)


def save(fig, name: str) -> None:
    fig.tight_layout()
    fig.savefig(OUT_DIR / name)
    plt.close(fig)
    print(f"  wrote {OUT_DIR / name}")


def main() -> None:
    OUT_DIR.mkdir(exist_ok=True)
    df = load()
    experiments = set(df["experiment"])
    print(f"Read {len(df)} rows from {CSV_PATH}")

    # Figure 1: scaling in n.
    if "vary_n" in experiments:
        d = summarise(df, "vary_n", "n")
        d["n_millions"] = d["n"] / 1e6
        fig, ax_t, ax_c = two_panel("Experiment 1: random text (σ = 4, m = 10), growing n",
                                    d, "n_millions", "Text length n (millions)")
        ax_c.axhline(2, color="#0b0b0b", linewidth=1.3, linestyle=":", label="KMP bound: 2", zorder=5)
        ax_c.set_ylim(0, 2.2)
        ax_t.legend(loc="upper left")
        ax_c.legend(loc="lower right")
        save(fig, "fig1_vary_n.png")

    # Figure 2: adversarial input.
    if "adversarial_vary_m" in experiments:
        d = summarise(df, "adversarial_vary_m", "m")
        fig, ax_t, ax_c = two_panel("Experiment 2: adversarial text (aaa…a, pattern aa…ab), growing m",
                                    d, "m", "Pattern length m", logx=True, logy_time=True)
        ax_c.set_yscale("log")
        n = d["n"].iloc[0]
        ms = sorted(d["m"].unique())
        add_theory_line(ax_c, ms, [(n - m + 1) * m / n for m in ms], "naive", "Naive theory: (n−m+1)·m / n")
        add_theory_line(ax_c, ms, [2] * len(ms), "kmp", "KMP bound: 2")
        ax_t.legend(loc="upper left")
        ax_c.legend(loc="upper left")
        save(fig, "fig2_adversarial.png")

    # Figure 3: random text, same m values.
    if "random_vary_m" in experiments:
        d = summarise(df, "random_vary_m", "m")
        sigma = int(df[df["experiment"] == "random_vary_m"]["sigma"].iloc[0])
        fig, ax_t, ax_c = two_panel(f"Experiment 3: random text (σ = {sigma}), growing m",
                                    d, "m", "Pattern length m", logx=True)
        ms = sorted(d["m"].unique())
        add_theory_line(ax_c, ms, [sigma / (sigma - 1)] * len(ms), "naive",
                        f"Naive expected: σ/(σ−1) ≈ {sigma / (sigma - 1):.2f}")
        add_theory_line(ax_c, ms, [2] * len(ms), "kmp", "KMP bound: 2")
        ax_c.set_ylim(0, 2.2)
        ax_t.legend(loc="lower right")
        ax_c.legend(loc="lower right")
        save(fig, "fig3_random_vary_m.png")

    # Figure 4: alphabet size.
    if "vary_sigma" in experiments:
        d = summarise(df, "vary_sigma", "sigma")
        fig, ax_t, ax_c = two_panel("Experiment 4: random text (m = 20), growing alphabet size σ",
                                    d, "sigma", "Alphabet size σ")
        sigmas = sorted(d["sigma"].unique())
        fine = [s / 10 for s in range(int(min(sigmas) * 10), int(max(sigmas) * 10) + 1)]
        add_theory_line(ax_c, fine, [s / (s - 1) for s in fine], "naive", "Naive expected: σ/(σ−1)")
        ax_c.set_ylim(0, 2.2)
        ax_t.legend(loc="upper right")
        ax_c.legend(loc="lower right")
        save(fig, "fig4_vary_sigma.png")

    # Figure 5: English text.
    if "english_vary_m" in experiments:
        d = summarise(df, "english_vary_m", "m")
        n = d["n"].iloc[0]
        fig, ax_t, ax_c = two_panel(f"Experiment 5: English text (n = {n:,}), growing m",
                                    d, "m", "Pattern length m", logx=True)
        ax_c.axhline(2, color="#0b0b0b", linewidth=1.3, linestyle=":", label="KMP bound: 2", zorder=5)
        ax_c.set_ylim(0, 2.2)
        ax_t.legend(loc="lower right")
        ax_c.legend(loc="lower right")
        save(fig, "fig5_english.png")

    # Figure 6: who wins? Naive time divided by KMP time, for every m-varying experiment.
    ratio_experiments = [("adversarial_vary_m", "Adversarial", "#4a3aa7"),
                         ("random_vary_m", "Random (σ = 4)", "#1baf7a"),
                         ("english_vary_m", "English", "#eda100")]
    ratio_experiments = [r for r in ratio_experiments if r[0] in experiments]
    if ratio_experiments:
        fig, ax = plt.subplots(figsize=(7, 4))
        fig.suptitle("Speed-up of KMP over naive (above 1 = KMP faster)",
                     x=0.01, ha="left", fontsize=12, color="#0b0b0b")
        for exp, label, color in ratio_experiments:
            d = summarise(df, exp, "m").pivot(index="m", columns="algorithm", values="time_ms")
            ax.plot(d.index, d["naive"] / d["kmp"], color=color, linewidth=2,
                    marker="o", markersize=5, label=label)
        ax.axhline(1, color=INK, linewidth=1)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Pattern length m")
        ax.set_ylabel("Naive time / KMP time")
        ax.legend(loc="upper left")
        save(fig, "fig6_speedup.png")

    # Figure 7: how much of KMP's time is preprocessing?
    if "random_vary_m" in experiments:
        d = summarise(df, "random_vary_m", "m")
        d = d[d["algorithm"] == "kmp"]
        fig, ax = plt.subplots(figsize=(7, 4))
        fig.suptitle("Share of KMP's time spent building the LPS array (random text, n = 1,000,000)",
                     x=0.01, ha="left", fontsize=12, color="#0b0b0b")
        ax.plot(d["m"], 100 * d["preprocess_ms"] / d["time_ms"], color=COLORS["kmp"],
                linewidth=2, marker="o", markersize=5)
        ax.set_xscale("log")
        ax.set_xlabel("Pattern length m")
        ax.set_ylabel("Preprocessing share of total time (%)")
        ax.set_ylim(bottom=0)
        save(fig, "fig7_preprocessing_share.png")

    # Console summary: one line per setting, for quick reading and the report's tables.
    print("\nSummary (median time in ms, mean comparisons per character):")
    for exp, x in [("vary_n", "n"), ("adversarial_vary_m", "m"), ("random_vary_m", "m"),
                   ("vary_sigma", "sigma"), ("english_vary_m", "m")]:
        if exp not in experiments:
            continue
        d = summarise(df, exp, x)
        table = d.pivot(index=x, columns="algorithm", values=["time_ms", "comparisons_per_char"])
        table.columns = [f"{alg}_{metric}".replace("time_ms", "ms").replace("comparisons_per_char", "cmp/n")
                         for metric, alg in table.columns]
        print(f"\n{exp}")
        print(table.round(3).to_string())

    # Save the same summary as a CSV for the report.
    summary = pd.concat([summarise(df, exp, x).assign(experiment=exp, x_name=x).rename(columns={x: "x"})
                         for exp, x in [("vary_n", "n"), ("adversarial_vary_m", "m"),
                                        ("random_vary_m", "m"), ("vary_sigma", "sigma"),
                                        ("english_vary_m", "m")] if exp in experiments])
    summary.to_csv(OUT_DIR / "summary.csv", index=False)
    print(f"\n  wrote {OUT_DIR / 'summary.csv'}")


if __name__ == "__main__":
    main()
