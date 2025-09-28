import sys

def get_buffer_size(N):
    k = (N + 6)
    if N >= 16 and N <= 64:
        k -= 5
    if N >= 190:
        k += 190 ** (N / 190)
    medium = (1.04 * 10**9) / k
    return (int(medium * 1.8), int(medium * 2.2))

N = int(sys.argv[1])
min_bs, max_bs = get_buffer_size(N)
print(min_bs, max_bs)
