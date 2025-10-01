import csv
import random

# Parameters
input_file = "data_fault/log_single_Raben.csv"
output_file = "data_fault/log_single_Raben_clean.csv"
Ns = [4, 8, 16, 32]
samples_per_class = 10

# Read data
with open(input_file, newline='', encoding='utf-8') as f:
    reader = csv.DictReader(f, delimiter=';')
    data = list(reader)

# Group rows by N and killed condition
filtered = []
for N in Ns:
    rows_for_N = [row for row in data if int(row["N"]) == N]

    # Split by killed condition
    killed_0 = [row for row in rows_for_N if int(row["KILLED"]) == 0]
    killed_12 = [row for row in rows_for_N if int(row["KILLED"]) in (1, 2)]

    # Randomly sample up to 10 from each group
    sampled_0 = random.sample(killed_0, min(samples_per_class, len(killed_0)))
    sampled_12 = random.sample(killed_12, min(samples_per_class, len(killed_12)))

    filtered.extend(sampled_0)
    filtered.extend(sampled_12)

# Write cleaned dataset
with open(output_file, "w", newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=data[0].keys(), delimiter=';')
    writer.writeheader()
    writer.writerows(filtered)

print(f"Saved cleaned dataset to {output_file}")