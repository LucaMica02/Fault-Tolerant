import csv

def clean_data(input_file, output_file):
    Ns = [4, 8, 16, 32]
    samples_per_class = 50

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
        killed_1 = [row for row in rows_for_N if int(row["KILLED"]) == 1]
        print(f"For N = {N}; killed_0 size = {len(killed_0)}; killed_1 size = {len(killed_1)}")

        # Take the first 50 from each group
        sampled_0 = killed_0[:samples_per_class]
        sampled_1 = killed_1[:samples_per_class]

        filtered.extend(sampled_0)
        filtered.extend(sampled_1)

    # Write cleaned dataset
    with open(output_file, "w", newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=data[0].keys(), delimiter=';')
        writer.writeheader()
        writer.writerows(filtered)

    print(f"Saved cleaned dataset to {output_file}")

# Example Usage
RD_inp = "../data/data_fault/log_single_RD.csv"
RD_out = "../data/data_fault/log_single_RD_clean.csv"
Raben_inp = "../data/data_fault/log_single_Raben.csv"
Raben_out = "../data/data_fault/log_single_Raben_clean.csv"

clean_data(RD_inp, RD_out)
clean_data(Raben_inp, Raben_out)