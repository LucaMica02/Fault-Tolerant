import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import numpy as np

def human_readable_size(size_bytes):
    """Convert size in bytes to human-readable string."""
    size_bytes *= 4
    if size_bytes < 1024:
        return f"{size_bytes}B"
    elif size_bytes < 1024**2:
        return f"{size_bytes // 1024}KB"
    elif size_bytes < 1024**3:
        return f"{size_bytes // 1024**2}MB"
    else:
        return f"{size_bytes // 1024**3}GB"

def plot_experiment_with_ratio(main_csv, original_csv, label, num_xticks=8):
    # Load CSV files
    data = pd.read_csv(main_csv, sep=";")
    original = pd.read_csv(original_csv, sep=";")

    # Group and average
    data_avg = data.groupby(["NP", "SIZE"], as_index=False)["TIME"].mean()
    original_avg = original.groupby(["NP", "SIZE"], as_index=False)["TIME"].mean()

    # Merge for comparison
    merged = pd.merge(
        data_avg, original_avg, on=["NP", "SIZE"], suffixes=(f"_{label}", "_ORIGINAL")
    )

    np_values = sorted(merged["NP"].unique())
    ncols = 2
    # Add one extra row if needed to fit ratio plot
    nrows = (len(np_values) + ncols) // ncols
    fig, axes = plt.subplots(nrows=nrows, ncols=ncols, figsize=(12, 4 * nrows))
    axes = axes.flatten()

    # Determine x-ticks
    all_sizes = np.array(sorted(merged["SIZE"].unique()))
    tick_indices = np.linspace(0, len(all_sizes) - 1, num_xticks, dtype=int)
    tick_sizes = all_sizes[tick_indices]
    tick_labels = [human_readable_size(s) for s in tick_sizes]

    # Plot each NP
    for i, np_val in enumerate(np_values):
        subset = merged[merged["NP"] == np_val].sort_values("SIZE")
        ax = axes[i]

        ax.plot(subset["SIZE"], subset[f"TIME_{label}"], marker="o", label=label)
        ax.plot(subset["SIZE"], subset["TIME_ORIGINAL"], marker="s", label=f"Original {label}")

        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.yaxis.set_major_formatter(ScalarFormatter())
        ax.ticklabel_format(style='plain', axis='y')
        ax.set_xlabel("Message size")
        ax.set_ylabel("Avg time (s)")
        ax.set_title(f"NP = {np_val}")
        ax.legend()
        ax.grid(True)
        ax.set_xticks(tick_sizes)
        ax.set_xticklabels(tick_labels, rotation=45, ha="right")

    # Plot ratio in the last subplot (bottom-right)
    ratio_ax = axes[-1]
    for np_val in np_values:
        subset = merged[merged["NP"] == np_val].sort_values("SIZE")
        ratio = subset[f"TIME_{label}"] / subset["TIME_ORIGINAL"]
        ratio_ax.plot(subset["SIZE"], ratio, marker="o", label=f"NP={np_val}")


    ratio_ax.set_xscale("log")
    ratio_ax.set_xlabel("Message size")
    ratio_ax.set_ylabel("Time ratio (custom / original)")
    ratio_ax.set_title("Ratio vs Original per NP")
    ratio_ax.grid(True, linestyle="--", alpha=0.7)
    ratio_ax.set_xticks(tick_sizes)
    ratio_ax.set_xticklabels(tick_labels, rotation=45, ha="right")
    ratio_ax.legend()
    ratio_ax.set_yticks([i for i in range(1, 10)])

    # Hide any unused subplots
    for j in range(len(np_values), len(axes)-1):
        fig.delaxes(axes[j])

    plt.tight_layout()
    plt.show()


# Example usage
plot_experiment_with_ratio("../data/data_compare/rd.csv", "../data/data_compare/original_rd.csv", "Recursive Doubling")
plot_experiment_with_ratio("../data/data_compare/raben.csv", "../data/data_compare/original_raben.csv", "Rabenseifner")