import sys

"""
This script take as input N and return a buffer size value.
The goal is to make the executation time between 2 and 4 seconds, 
in order to make simulation of killing process randmly.
"""

def get_buffer_size(N):
    # Start by defining k as N+6
    k = (N + 6)

    # If N is between 16 and 64 (inclusive), subtract 5 from k
    if N >= 16 and N <= 64:
        k -= 5

    # If N is very large (>=190), increase k dramatically
    # by adding 190^(N/190), which grows exponentially with N
    if N >= 190:
        k += 190 ** (N / 190)

    # Define a "medium" value: about 1.04 billion divided by k
    medium = (1.04 * 10**9) / k

    # Return a tuple of two integers: scaled versions of medium
    # Lower bound: ~1.8 * medium
    # Upper bound: ~2.2 * medium
    return (int(medium * 1.8), int(medium * 2.2))


# Script entry point: read N from the command line
N = int(sys.argv[1])
min_bs, max_bs = get_buffer_size(N)

# Print the buffer size range
print(min_bs, max_bs)