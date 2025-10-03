import os, csv

### DEFINE THE FILES PATH HERE ###
RD_FILE = "../out/rd.txt"
RD_ORIGINAL_FILE = "../out/original_rd.txt"
RABEN_FILE = "../out/raben.txt"
RABEN_ORIGINAL_FILE = "../out/original_raben.txt"
RD_FILE_CSV = "../data/data_compare_block/rd.csv"
RD_ORIGINAL_FILE_CSV = "../data/data_compare_block/original_rd.csv"
RABEN_FILE_CSV = "../data/data_compare_block/raben.csv"
RABEN_ORIGINAL_FILE_CSV = "../data/data_compare_block/original_raben.csv"
ERROR_FILE = "../out/error.txt"

# Read output data from the file
def read(file):
    results = []
    np, size, time = None, None, None
    with open(file, "r") as file:
        lines = file.readlines()
    for line in lines:
        line = line.split()
        if line[0] == "Hello":
            results.append(int(line[-1]))
        elif line[0] == "P:":
            np = int(line[-1])
        elif line[0] == "Size:":
            size = int(line[-1]) 
        elif line[0] == "Time:":
            time = float(line[-1])
    return np, size, time, results

# Check if the results are correct
def check(res1, res2, np):
    if len(res1) != np or len(res2) != np:
        return False
    target = res1[0]
    for i in range(np):
        if res1[i] != target or res2[i] != target:
            return False 
    return True

# Append the data to the log file
def append(file, np, size, time, result):
    headers = ["NP", "SIZE", "TIME", "RESULT"]
    if not os.path.exists(file):
        with open(file, 'w', newline='') as f:
            writer = csv.writer(f, delimiter=';')
            writer.writerow(headers) 
    with open(file, 'a', newline='') as f:
        writer = csv.writer(f, delimiter=';')
        writer.writerow([np, size, time, result])

# Keep track of the errors
def append_error(file, row1, row2):
    with open(file, 'a') as f:
        f.write(row1 + "\n")
        f.write(row2 + "\n")
        f.write("#############################################\n")

def main():
    np_rd, size_rd, time_rd, results_rd = read(RD_FILE)
    np_rd_o, size_rd_o, time_rd_o, results_rd_o = read(RD_ORIGINAL_FILE)
    np_raben, size_raben, time_raben, results_raben = read(RABEN_FILE)
    np_raben_o, size_raben_o, time_raben_o, results_raben_o = read(RABEN_ORIGINAL_FILE)

    # LOG
    print(f"NP: {np_rd}, SIZE: {size_rd}, RESULT: {results_rd[0]}")

    # Check RD
    if check(results_rd, results_rd_o, np_rd):
        append(RD_FILE_CSV, np_rd, size_rd, time_rd, results_rd[0])
        append(RD_ORIGINAL_FILE_CSV, np_rd_o, size_rd_o, time_rd_o, results_rd_o[0])
        print(f"RD: {time_rd}, RD_ORG: {time_rd_o}")
    else:
        append_error(ERROR_FILE, "RD: " + str([np_rd, size_rd, time_rd, results_rd]), "RD_O: " + str([np_rd_o, size_rd_o, time_rd_o, results_rd_o]))
        print("########### ERROR WITH RD ###########")

    # Check Raben
    if check(results_raben, results_raben_o, np_raben):
        append(RABEN_FILE_CSV, np_raben, size_raben, time_raben, results_raben[0])
        append(RABEN_ORIGINAL_FILE_CSV, np_raben_o, size_raben_o, time_raben_o, results_raben_o[0])
        print(f"RABEN: {time_raben}, RABEN_ORG: {time_raben_o}")
    else:
        append_error(ERROR_FILE, "Raben: " + str([np_raben, size_raben, time_raben, results_raben]), "Raben_O: " + str([np_raben_o, size_raben_o, time_raben_o, results_raben_o]))
        print("########### ERROR WITH RABEN ###########")
    
    print("####################################################")

if __name__ == "__main__":
    main()