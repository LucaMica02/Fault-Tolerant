import os, csv, sys

def append_with_separator(algo, input_file1, input_file2, input_file3, text, output_file):
    # Create the output file if it doesn't exist
    if not os.path.exists(output_file):
        open(output_file, 'w').close()

    # Read from input and append to output
    with open(input_file1, 'r') as inp1, open(input_file2, 'r') as inp2, open(input_file3, 'r') as inp3, open(output_file, 'a') as out:
        out.write(f"Algo Used: {algo}\n")
        out.write(inp1.read())
        out.write('\n')
        out.write(inp2.read())
        out.write('\n')
        out.write(inp3.read())
        out.write('\n' + text)
        out.write('\n' + "######################################################################")

# take the parameters from test_log  
def getParameters():
    DEADLOCK = False
    TIME_T = None
    TIME = None
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
                DELAY = float(line[-1])
            elif line[0] == "TIMEOUT":
                TIMEOUT = int(line[-1])
            elif line[0] == "real":
                TIME_T = float(line[-1][2:-1])
            elif line[0] == "MPI_ABORT" or "MPI_ERRORS_ARE_FATAL" in line:
                ABORT = True
    result = calcExpectedRes(N-1, BUF_SIZE)
    KILLED, RIGHT_RESULT, TIME = mpiOutput(N, result)
    if not TIME:
        TIME = TIME_T
    if TIME_T > TIMEOUT:
        DEADLOCK = True
    with open("../out/check.txt", "w") as f:
        if KILLED >= 1 and KILLED <= 2 and RIGHT_RESULT == True and DEADLOCK == False:
            f.write("True")
        else:
            f.write("False")
    return [N, DELAY, BUF_SIZE, KILLED, TIME, DEADLOCK, SEGFAULT, ABORT, RIGHT_RESULT]

# Calculate the expected result 
def calcExpectedRes(N, BUF_SIZE):
    sum_ = (N * (N+1)) / 2
    mod = 17
    result = (sum_ % mod) * BUF_SIZE
    print(result, int(result))
    return int(result)

# Read all the results from mpi_out
def mpiOutput(N, result):
    RIGHT_RESULT = True
    TIME = None
    killed = []
    survivors = set()
    with open("../out/mpi_out.txt", "r") as file:
        lines = file.readlines()
    for line in lines:
        line = line.split()
        if line[0] == "Hello":
            survivors.add(int(line[2]))
            if int(line[-1]) != result:
                RIGHT_RESULT = False
        elif line[0] == "Time:":
            TIME = float(line[-1])
    for i in range(N):
        if i not in survivors:
            killed.append(i)
    return len(killed), RIGHT_RESULT, TIME

all_reduce_type = sys.argv[1]
log_file = sys.argv[2]
parameters = getParameters()
print(parameters)
if parameters[5] == True or parameters[8] == False:
    append_with_separator(all_reduce_type, "../out/mpi_out.txt", "../out/docker_out.txt", "../out/test_log.txt", str(parameters), "../out/log_errors.txt")
    print("########################### ERROR ###########################")

# Write on the csv file
filename = log_file
headers = ["N", "DELAY", "BUF SIZE", "KILLED", "TIME", "DEADLOCK", "SEGFAULT", "ABORT", "RIGHT RESULT"]

# If not exists create the file and write the headers
if not os.path.exists(filename):
    with open(filename, 'w', newline='') as file:
        writer = csv.writer(file, delimiter=';')
        writer.writerow(headers)

# Append the data
with open(filename, 'a', newline='') as file:
    writer = csv.writer(file, delimiter=';')
    writer.writerow(parameters)
