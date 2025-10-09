import pandas as pd
import matplotlib.pyplot as plt

def plot_data(input_file, algo_name="Algorithm"):
    # Read CSV
    df = pd.read_csv(input_file, delimiter=";")

    # Convert columns to numeric
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["KILLED"] = pd.to_numeric(df["KILLED"], errors="coerce")
    df["TIME"] = pd.to_numeric(df["TIME"], errors="coerce")

    # Group into KILLED=0 vs KILLED=1
    df["KILLED_GROUP"] = df["KILLED"].apply(lambda x: 0 if x == 0 else 1)

    # Compute mean for each (N, KILLED_GROUP)
    stats = df.groupby(["N", "KILLED_GROUP"])["TIME"].mean().reset_index()

    # Pivot for mean
    pivot_mean = stats.pivot(index="N", columns="KILLED_GROUP", values="TIME").reset_index()
    pivot_mean = pivot_mean.rename(columns={0: "MEAN_KILLED_0", 1: "MEAN_KILLED_1"})

    # ---- Plot ----
    plt.figure(figsize=(10,6))

    # Raw scatter points (all runs)
    colors = {0: "blue", 1: "red"}
    for group in [0, 1]:
        subset = df[df["KILLED_GROUP"] == group]
        plt.scatter(subset["N"], subset["TIME"], alpha=0.3, s=20,
                    color=colors[group], label=f"KILLED={group} raw")

    # Mean lines (no std)
    plt.plot(pivot_mean["N"], pivot_mean["MEAN_KILLED_0"], 
             "-o", color="blue", linewidth=2, label="KILLED=0 mean")

    plt.plot(pivot_mean["N"], pivot_mean["MEAN_KILLED_1"], 
             "-o", color="red", linewidth=2, label="KILLED=1 mean")

    # Labels and formatting
    plt.xlabel("NP (number of processes)")
    plt.ylabel("TIME (in seconds)")
    plt.title(f"{algo_name}: Execution TIME per NP (raw + mean)")
    plt.legend()
    plt.grid(True)
    xticks = [4, 8, 12, 16, 20, 24, 28, 32]  
    plt.xticks(ticks=xticks, labels=xticks) 
    plt.show()

# Example usage
plot_data("../data/data_fault/log_single_RD_clean.csv", algo_name="Recursive Doubling")
plot_data("../data/data_fault/log_single_Raben_clean.csv", algo_name="Rabenseifner")