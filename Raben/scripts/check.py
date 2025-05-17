import sys
import os
import csv

def calcExpectedRes(N, BUF_SIZE):
    sum_ = (N * (N+1)) / 2
    mod = 17
    result = (sum_ % mod) * BUF_SIZE
    return int(result)

def checkRightResult(res):
    RIGHT_RESULT = True
    SEGFAULT = False
    ABORT = False
    TIME = -1
    survived = 0
    with open("../out/mpi_out.txt", "r") as file:
        lines = file.readlines()
        for line in lines:
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
    return RIGHT_RESULT, SEGFAULT, ABORT, TIME, survived

N = int(sys.argv[1])
DELAY = int(sys.argv[2])
BUF_SIZE = int(sys.argv[3])
TIMEOUT = int(sys.argv[4])
res = calcExpectedRes(N-1, BUF_SIZE)
RIGHT_RESULT, SEGFAULT, ABORT, TIME, survived = checkRightResult(res)
MPI_KILLED = N - survived
DEADLOCK = TIME > 30
print(res)
print(N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, SEGFAULT, ABORT, RIGHT_RESULT, MPI_KILLED)

'''
[N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, SEGFAULT, ABORT, RIGHT RESULT, REAL MPI KILLED]
 *    *       *         *       *      *          *       *         *               *
'''

# Write on the csv file
filename = "../log.csv"
headers = ["N", "DELAY", "BUF_SIZE", "TIMEOUT", "TIME", "DEADLOCK", "SEGFAULT", "ABORT", "RIGHT RESULT", "KILLED"]

# If not exists create the file and write the headers
if not os.path.exists(filename):
    with open(filename, 'w', newline='') as file:
        writer = csv.writer(file, delimiter=';')
        writer.writerow(headers)

# Append the data
with open(filename, 'a', newline='') as file:
    writer = csv.writer(file, delimiter=';')
    writer.writerow([N, DELAY, BUF_SIZE, TIMEOUT, TIME, DEADLOCK, SEGFAULT, ABORT, RIGHT_RESULT, MPI_KILLED])