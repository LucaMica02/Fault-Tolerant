import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

def plot_data_boxplot_clean(input_file, algo_name="Algorithm"):
    df = pd.read_csv(input_file, delimiter=";")
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["KILLED"] = pd.to_numeric(df["KILLED"], errors="coerce")
    df["TIME"] = pd.to_numeric(df["TIME"], errors="coerce")
    df = df.dropna(subset=["N", "KILLED", "TIME"])

    df = df[df["TIME"] < 5]
    Ns = sorted(df["N"].unique())
    width = 0.35  # width of each box
    positions = np.arange(len(Ns))  # base positions for each N

    # Prepare data per KILLED group
    data0 = [df[(df["N"] == n) & (df["KILLED"] == 0)]["TIME"] for n in Ns]
    data1 = [df[(df["N"] == n) & (df["KILLED"] == 1)]["TIME"] for n in Ns]

    plt.figure(figsize=(12, 6))

    meanprops = dict(marker='D', markeredgecolor='black', markerfacecolor='black', markersize=7)

    """
    # Print full dataset
    grouped = df.groupby(["N", "KILLED"])["TIME"].agg(["count", "mean", "median", "std", "max"])
    print(f"\n=== Summary per N & KILLED for {algo_name} ===")
    print(grouped.to_string(float_format="{:.3f}".format))
    """

    # Plot boxes side by side with mean
    plt.boxplot(data0, positions=positions - width/2, widths=width, patch_artist=True,
                showmeans=True, meanprops=meanprops,
                boxprops=dict(facecolor="#1f77b4", alpha=0.8, linewidth=1.5),
                medianprops=dict(color="black", linewidth=1.5),
                whiskerprops=dict(linewidth=1.2),
                capprops=dict(linewidth=1.2))
    
    plt.boxplot(data1, positions=positions + width/2, widths=width, patch_artist=True,
                showmeans=True, meanprops=meanprops,
                boxprops=dict(facecolor="#ff7f0e", alpha=0.8, linewidth=1.5),
                medianprops=dict(color="black", linewidth=1.5),
                whiskerprops=dict(linewidth=1.2),
                capprops=dict(linewidth=1.2))

    plt.xticks(positions, [str(n) for n in Ns])
    plt.xlabel("Number of Processes (NP)")
    plt.ylabel("Execution Time (seconds)")
    plt.title(f"{algo_name} Runtime: Zero vs One Killed Process")
    plt.yscale("log")
    #plt.grid(axis="y", linestyle="--", alpha=0.7)

    # Legend
    blue_patch = mpatches.Patch(color="#1f77b4", alpha=0.8, label="Without Failures")
    red_patch = mpatches.Patch(color="#ff7f0e", alpha=0.8, label="With Failures")
    plt.legend(handles=[blue_patch, red_patch], loc="upper left")

    # Add explanation text box
    explanation_text = (
        "Box: 25th-75th percentile (IQR)\n"
        "Black line: Median\n"
        "Black diamond: Mean\n"
        "Whiskers: min/max within 1.5*IQR\n"
        "Circles: Outliers / raw data points"
    )
    plt.gcf().text(0.80, 0.12, explanation_text, fontsize=9,
                   bbox=dict(facecolor='white', alpha=0.7, edgecolor='black'))
    plt.tight_layout()
    plt.show()


# Example usage
plot_data_boxplot_clean("../data/data_fault/log_single_RD_clean.csv", algo_name="Recursive Doubling")
plot_data_boxplot_clean("../data/data_fault/log_single_Raben_clean.csv", algo_name="Rabenseifner")