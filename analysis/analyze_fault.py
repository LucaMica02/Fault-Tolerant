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

    Ns = sorted(df["N"].unique())
    width = 0.35  # width of each box
    positions = np.arange(len(Ns))  # base positions for each N

    # Prepare data per KILLED group
    data0 = [df[(df["N"] == n) & (df["KILLED"] == 0)]["TIME"] for n in Ns]
    data1 = [df[(df["N"] == n) & (df["KILLED"] == 1)]["TIME"] for n in Ns]

    plt.figure(figsize=(12, 6))

    # Plot boxes side by side
    plt.boxplot(data0, positions=positions - width/2, widths=width, patch_artist=True,
                boxprops=dict(facecolor="#4a90e2", alpha=0.5, linewidth=1.5),
                medianprops=dict(color="black", linewidth=1.5),
                whiskerprops=dict(linewidth=1.2),
                capprops=dict(linewidth=1.2))
    
    plt.boxplot(data1, positions=positions + width/2, widths=width, patch_artist=True,
                boxprops=dict(facecolor="#e94e4e", alpha=0.5, linewidth=1.5),
                medianprops=dict(color="black", linewidth=1.5),
                whiskerprops=dict(linewidth=1.2),
                capprops=dict(linewidth=1.2))

    plt.xticks(positions, [str(n) for n in Ns])
    plt.xlabel("Number of Processes (N)")
    plt.ylabel("Execution Time (seconds)")
    plt.title(f"{algo_name}: Execution TIME per NP (Boxplot)")
    plt.grid(axis="y", linestyle="--", alpha=0.7)

    # Legend
    blue_patch = mpatches.Patch(color="#4a90e2", alpha=0.5, label="KILLED=0")
    red_patch = mpatches.Patch(color="#e94e4e", alpha=0.5, label="KILLED=1")
    plt.legend(handles=[blue_patch, red_patch], loc="upper left")

    plt.tight_layout()
    plt.show()


# Example usage
plot_data_boxplot_clean("../data/data_fault/log_single_RD_clean.csv", algo_name="Recursive Doubling")
plot_data_boxplot_clean("../data/data_fault/log_single_Raben_clean.csv", algo_name="Rabenseifner")