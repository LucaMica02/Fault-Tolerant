import pandas as pd
import matplotlib.pyplot as plt

def plot_data(input_file):
    # Read CSV
    df = pd.read_csv(input_file, delimiter=";")

    # Convert columns to numeric
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["KILLED"] = pd.to_numeric(df["KILLED"], errors="coerce")
    df["TIME"] = pd.to_numeric(df["TIME"], errors="coerce")

    # Group into KILLED=0 vs KILLED=1
    df["KILLED_GROUP"] = df["KILLED"].apply(lambda x: 0 if x == 0 else 1)

    # Compute mean and std for each (N, KILLED_GROUP)
    stats = df.groupby(["N", "KILLED_GROUP"])["TIME"].agg(["mean", "std"]).reset_index()

    # Pivot for mean and std
    pivot_mean = stats.pivot(index="N", columns="KILLED_GROUP", values="mean").reset_index()
    pivot_std  = stats.pivot(index="N", columns="KILLED_GROUP", values="std").reset_index()

    pivot_mean = pivot_mean.rename(columns={0: "MEAN_KILLED_0", 1: "MEAN_KILLED_1"})
    pivot_std  = pivot_std.rename(columns={0: "STD_KILLED_0", 1: "STD_KILLED_1"})

    # ---- Plot ----
    plt.figure(figsize=(10,6))

    # Raw scatter points (all runs)
    colors = {0: "blue", 1: "red"}
    for group in [0, 1]:
        subset = df[df["KILLED_GROUP"] == group]
        plt.scatter(subset["N"], subset["TIME"], alpha=0.3, s=20,
                    color=colors[group], label=f"KILLED={group} raw")

    # Mean ± std (error bars + line)
    plt.errorbar(pivot_mean["N"], pivot_mean["MEAN_KILLED_0"], 
                 yerr=pivot_std["STD_KILLED_0"], fmt="-o", capsize=5,
                 color="blue", linewidth=2, label="KILLED=0 (avg ± std)")

    plt.errorbar(pivot_mean["N"], pivot_mean["MEAN_KILLED_1"], 
                 yerr=pivot_std["STD_KILLED_1"], fmt="-o", capsize=5,
                 color="red", linewidth=2, label="KILLED=1 (avg ± std)")

    plt.xlabel("N")
    plt.ylabel("TIME")
    plt.title("Execution TIME per N (raw + mean ± std)")
    plt.legend()
    plt.grid(True)
    plt.show()

# Parameters
RD_file = "data_fault_block/log_single_RD_clean.csv"
raben_file = "data_fault_block/log_single_Raben_clean.csv"
plot_data(RD_file)
plot_data(raben_file)