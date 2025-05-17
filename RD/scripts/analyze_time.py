'''
def getAvg(filename):
    time = 0
    count = 0
    with open(filename, 'r') as file:
        for line in file:
            time += float(line.split()[1])
            count += 1
    return time / count

print("SCALABILITA' SU P")
print(f'P = 4; v1: {getAvg("../tests/v1/time_4_5.txt")}; v2: {getAvg("../tests/v2/time_4_5.txt")}')
print(f'P = 8; v1: {getAvg("../tests/v1/time_8_5.txt")}; v2: {getAvg("../tests/v2/time_8_5.txt")}')
print(f'P = 16; v1: {getAvg("../tests/v1/time_16_5.txt")}; v2: {getAvg("../tests/v2/time_16_5.txt")}')
print(f'P = 32; v1: {getAvg("../tests/v1/time_32_5.txt")}; v2: {getAvg("../tests/v2/time_32_5.txt")}')

print("\n#######################################################\n")

print("SCALABILITA' SU N")
print(f'N = 10000000; v1: {getAvg("../tests/v1/time_32_1.txt")}; v2: {getAvg("../tests/v2/time_32_1.txt")}')
print(f'N = 20000000; v1: {getAvg("../tests/v1/time_32_2.txt")}; v2: {getAvg("../tests/v2/time_32_2.txt")}')
print(f'N = 40000000; v1: {getAvg("../tests/v1/time_32_4.txt")}; v2: {getAvg("../tests/v2/time_32_4.txt")}')
print(f'N = 80000000; v1: {getAvg("../tests/v1/time_32_8.txt")}; v2: {getAvg("../tests/v2/time_32_8.txt")}')
'''

import matplotlib.pyplot as plt

def getAvg(filename):
    time = 0
    count = 0
    with open(filename, 'r') as file:
        for line in file:
            time += float(line.split()[1])
            count += 1
    return time / count

# ----------------------------
# SCALABILITY ON P
# ----------------------------
P_values = [4, 8, 16, 32]
v1_P = [getAvg(f"../tests/v1/time_{p}_5.txt") for p in P_values]
v2_P = [getAvg(f"../tests/v2/time_{p}_5.txt") for p in P_values]

plt.figure(figsize=(10, 5))
plt.plot(P_values, v1_P, marker='o', label='v1: Standard RD')
plt.plot(P_values, v2_P, marker='s', label='v2: My implementation of RD')
plt.title("SCALABILITY ON P")
plt.xlabel("P (number of processes)")
plt.ylabel("Average Time (s)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# ----------------------------
# SCALABILITY ON N
# ----------------------------
N_values = [10_000_000, 20_000_000, 40_000_000, 80_000_000]
v1_N = [getAvg(f"../tests/v1/time_32_{i}.txt") for i in [1, 2, 4, 8]]
v2_N = [getAvg(f"../tests/v2/time_32_{i}.txt") for i in [1, 2, 4, 8]]

plt.figure(figsize=(10, 5))
plt.plot(N_values, v1_N, marker='o', label='v1: Standard RD')
plt.plot(N_values, v2_N, marker='s', label='v2: My implementation of RD')
plt.title("SCALABILITY ON N")
plt.xlabel("N (input size)")
plt.ylabel("Average Time (s)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
