import os, csv

# take the parameters from test_log  
def getParameters():
    DEADLOCK = False
    SEGFAULT = False
    ABORT = False
    RIGHT_RESULT = False
    with open("../out/test_log.txt", "r") as file:
        lines = file.readlines()
    for line in lines:
        if "Segmentation fault" in line or "(core dumped)" in line:
            SEGFAULT = True
        line = line.split()
        if line:
            if line[0] == "N":
                N = int(line[-1])
            elif line[0] == "BUF_SIZE":
                BUF_SIZE = int(line[-1])
            elif line[0] == "DELAY":
                DELAY = int(line[-1])
            elif line[0] == "THRESHOLD":
                THRESHOLD = int(line[-1])
            elif line[0] == "TIMEOUT":
                TIMEOUT = int(line[-1])
            elif line[0] == "real":
                TIME = float(line[-1][2:-1])
            elif line[0] == "MPI_ABORT" or "MPI_ERRORS_ARE_FATAL" in line:
                ABORT = True
    result = calcExpectedRes(N-1, BUF_SIZE)
    expectedKill = countKill()
    realKill, RIGHT_RESULT = mpiOutput(N, result)
    if TIME > TIMEOUT:
        DEADLOCK = True
    return [N, DELAY, THRESHOLD, BUF_SIZE, expectedKill, len(realKill), TIME, DEADLOCK, SEGFAULT, ABORT, RIGHT_RESULT]

# Calculate the expected result 
def calcExpectedRes(N, BUF_SIZE):
    sum_ = (N * (N+1)) / 2
    mod = 17
    result = (sum_ % mod) * BUF_SIZE
    print(result, int(result))
    return int(result)

# Read the expected kill from docker_out
def countKill():
    killed = []
    with open("../out/docker_out.txt", "r") as file:
        lines = file.readlines()
    return len(lines)

# Read all the results from mpi_out
def mpiOutput(N, result):
    RIGHT_RESULT = True
    killed = []
    survivors = set()
    with open("../out/mpi_out.txt", "r") as file:
        lines = file.readlines()
    for line in lines:
        line = line.split()
        survivors.add(int(line[2]))
        if int(line[-1]) != result:
            RIGHT_RESULT = False
    for i in range(N):
        if i not in survivors:
            killed.append(i)
    return killed, RIGHT_RESULT

parameters = getParameters()
print(parameters)

# Write on the csv file
filename = "../log.csv"
headers = ["N", "DELAY", "THRESHOLD", "BUF SIZE", "KILLED DOCKER", "REAL MPI KILLED", "TIME", "DEADLOCK", "SEGFAULT", "ABORT", "RIGHT RESULT"]

# If not exists create the file and write the headers
if not os.path.exists(filename):
    with open(filename, 'w', newline='') as file:
        writer = csv.writer(file, delimiter=';')
        writer.writerow(headers)

# Append the data
with open(filename, 'a', newline='') as file:
    writer = csv.writer(file, delimiter=';')
    writer.writerow(parameters)