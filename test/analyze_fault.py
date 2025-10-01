import pandas as pd
import matplotlib.pyplot as plt

# Parameters
input_file = "data_fault/log_single_RD_clean.csv"  # or your latest dataset

# Read CSV
df = pd.read_csv(input_file, delimiter=";")

# Convert columns to numeric
df["N"] = pd.to_numeric(df["N"], errors="coerce")
df["KILLED"] = pd.to_numeric(df["KILLED"], errors="coerce")
df["TIME"] = pd.to_numeric(df["TIME"], errors="coerce")

# Group into KILLED=0 vs KILLED>0
df["KILLED_GROUP"] = df["KILLED"].apply(lambda x: 0 if x == 0 else 1)

# Compute mean TIME for each (N, KILLED_GROUP)
avg_times = df.groupby(["N", "KILLED_GROUP"])["TIME"].mean().reset_index()

# Pivot to get both groups side by side
pivot = avg_times.pivot(index="N", columns="KILLED_GROUP", values="TIME").reset_index()
pivot.columns = ["N", "TIME_KILLED_0", "TIME_KILLED_GT0"]

# Plot
plt.figure(figsize=(8,5))
plt.plot(pivot["N"], pivot["TIME_KILLED_0"], marker="o", linewidth=2, label="KILLED = 0")
plt.plot(pivot["N"], pivot["TIME_KILLED_GT0"], marker="o", linewidth=2, label="KILLED > 0")

plt.xlabel("N")
plt.ylabel("Average TIME")
plt.title("Average TIME per N (Grouped by KILLED)")
plt.legend()
plt.grid(True)
plt.show()