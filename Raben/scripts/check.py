import sys
import os
import csv

def append_with_separator(input_file1, input_file2, text, output_file):
    # Create the output file if it doesn't exist
    if not os.path.exists(output_file):
        open(output_file, 'w').close()

    # Read from input and append to output
    with open(input_file1, 'r') as inp1, open(input_file2, 'r') as inp2, open(output_file, 'a') as out:
        out.write(inp1.read())
        out.write('\n')
        out.write(inp2.read())
        out.write('\n' + text)
        out.write('\n' + "######################################################################")

def log_file(filename):
    with open(filename, "r") as f:
        print(f.read())

def calcExpectedRes(N, BUF_SIZE):
    sum_ = (N * (N+1)) / 2
    mod = 17
    result = (sum_ % mod) * BUF_SIZE
    return int(result)

def checkRightResult(res):
    RIGHT_RESULT = True
    SEGFAULT = False
    ABORT = False
    DEADLOCK_DET = False
    WHERE = "UNKNOWN"
    TIME = -1
    survived = 0
    with open("../out/mpi_out.txt", "r") as file:
        lines = file.readlines()
        for line in lines:
            if "ALLGATHER" in line:
                WHERE = "ALLGATHER"
            if "REDUCE-SCATTER" in line:
                WHERE = "REDUCE-SCATTER"
            if "DEADLOCK" in line:
                DEADLOCK_DET = True
            if "Segmentation fault" in line or "(core dumped)" in line:
                SEGFAULT = True
            if "MPI_ABORT" in line or "MPI_ERRORS_ARE_FATAL" in line:
                ABORT = True
            line = line.split()
            if not line:
                continue
            elif line[0] != "Hello":
                if line[0] == "Time":
                    TIME = float(line[2])
            else:
                survived += 1
                if int(line[9]) != res:
                    RIGHT_RESULT = False
    return RIGHT_RESULT, SEGFAULT, ABORT, TIME, DEADLOCK_DET, survived, WHERE

N = int(sys.argv[1])
DELAY = int(sys.argv[2])
BUF_SIZE = int(sys.argv[3])
TIMEOUT = int(sys.argv[4])
res = calcExpectedRes(N-1, BUF_SIZE)
RIGHT_RESULT, SEGFAULT, ABORT, TIME, DEADLOCK_DET, survived, WHERE = checkRightResult(res)
MPI_KILLED = N - survived
DEADLOCK = TIME > 30
if DEADLOCK or not RIGHT_RESULT:
    append_with_separator("../out/mpi_out.txt", "../out/docker_out.txt", str(res) + " " + str([N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, DEADLOCK_DET, SEGFAULT, ABORT, RIGHT_RESULT, MPI_KILLED]), "../out/log_errors.txt")
    log_file("../out/mpi_out.txt")
    log_file("../out/docker_out.txt")
print(res)
print(N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, DEADLOCK_DET, SEGFAULT, ABORT, RIGHT_RESULT, MPI_KILLED, WHERE)

'''
[N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, DEADLOCK DETECTION, SEGFAULT, ABORT, RIGHT RESULT, REAL MPI KILLED]
 *    *       *         *       *      *          *       *         *               *
'''

# Write on the csv file
filename = "../logs/log.csv"
# headers = ["N", "DELAY", "BUF_SIZE", "TIMEOUT", "TIME", "DEADLOCK", "DEADLOCK DETECTION", "SEGFAULT", "ABORT", "RIGHT RESULT", "KILLED"]
headers = ["N", "DELAY", "BUF_SIZE", "TIMEOUT", "TIME", "DEADLOCK", "DEADLOCK DETECTION", "SEGFAULT", "ABORT", "RIGHT RESULT", "KILLED", "WHERE"]

# If not exists create the file and write the headers
if not os.path.exists(filename):
    with open(filename, 'w', newline='') as file:
        writer = csv.writer(file, delimiter=';')
        writer.writerow(headers)

# Append the data
with open(filename, 'a', newline='') as file:
    writer = csv.writer(file, delimiter=';')
    writer.writerow([N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, DEADLOCK_DET, SEGFAULT, ABORT, RIGHT_RESULT, MPI_KILLED, WHERE])
