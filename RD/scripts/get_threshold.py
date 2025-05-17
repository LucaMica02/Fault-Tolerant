import sys

def get_threshold(N):
    return 50 * (1 / (N / 4) ** (1 / 2))

N = int(sys.argv[1])
threshold = int(get_threshold(N))
print(threshold)