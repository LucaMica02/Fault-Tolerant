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

def plot_experiment(main_csv, original_csv, label, num_xticks=8):
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
    nrows = (len(np_values) + ncols - 1) // ncols
    fig, axes = plt.subplots(nrows=nrows, ncols=ncols, figsize=(12, 4 * nrows))
    axes = axes.flatten()

    # Determine 7-8 indicative x-tick positions
    all_sizes = np.array(sorted(merged["SIZE"].unique()))
    tick_indices = np.linspace(0, len(all_sizes) - 1, num_xticks, dtype=int)
    tick_sizes = all_sizes[tick_indices]
    tick_labels = [human_readable_size(s) for s in tick_sizes]

    for i, np_val in enumerate(np_values):
        subset = merged[merged["NP"] == np_val].sort_values("SIZE")
        ax = axes[i]

        # Plot all experimental points
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

    # Hide unused subplots
    for j in range(i + 1, len(axes)):
        fig.delaxes(axes[j])

    plt.tight_layout()
    plt.show()

# Example usage
plot_experiment("../data/data_compare/rd.csv", "../data/data_compare/original_rd.csv", "Recursive Dubling")
plot_experiment("../data/data_compare/raben.csv", "../data/data_compare/original_raben.csv", "Rabenseifner")
plot_experiment("../data/data_compare_block/rd.csv", "../data/data_compare_block/original_rd.csv", "Recursive Doubling")
plot_experiment("../data/data_compare_block/raben.csv", "../data/data_compare_block/original_raben.csv", "Rabenseifner")