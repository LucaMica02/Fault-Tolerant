# 🛡️ Fault-Tolerant Recursive Doubling

## ⚙️ ULFM (User-Level Failure Mitigation)

We are using **OpenMPI** with **ULFM** support. Compilation and execution are performed inside a Docker container.

### 🐳 Steps to Compile & Run

1. **Pull the Docker container**  
  To begin, you need to pull the Docker container with the following command:
   ```bash
   docker pull abouteiller/mpi-ft-ulfm
   ```
2. **Create the Makefile**
  Create a Makefile in your directory that will be used for compiling the program. You can refer to the Makefile in the repository’s src/ folder.
3. **Grant Docker permission to the directory**
  Ensure that the directory is accessible within the Docker container by granting the necessary permissions. This allows Docker to access and modify files as needed.
4. **Compile the program**
   To compile the code, run the following command, which mounts the current directory ($PWD) to the Docker container’s /sandbox folder and runs the make command:
   ```bash
   docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm make
   ```
5. **Run the program**
   To execute the program, use the mpirun command along with the appropriate flags for ULFM support. Replace <n> with the desired number of processes.
   ```bash
   docker run -v $PWD:/sandbox abouteiller/mpi-ft-ulfm mpirun --with-ft ulfm -np <n> ./main
   ```

## Project Structure

- `/src`
  - `main.c`: Code to test the recursive doubling
  - `recursive_doubling.c`: Actual implementation of recursive doubling with three different versions
  - `errhandler.c`: Functions that manage process failures
  - `util.c`: General-purpose utilities used in the codebase
  - `header.h`: Function, constant, and struct declarations
  - `Makefile`: Build file — run `make` to compile and `make clean` to clean up

- `/scripts`
  - `test.sh`: Chooses number of ranks and delay randomly; generates parameters for a ~5–15s execution
  - `run.sh`: Takes the parameters and runs the main program
  - `kill.sh`: Randomly kills ranks running in the Docker container (takes initial delay and threshold)
  - `get_threshold.py`: Returns the threshold value based on N
  - `get_bs.py`: Returns buffer size for given N
  - `check.py`: Verifies output correctness, detects deadlocks/aborts, and logs results
  - `analyze_log.py`: Analyzes log files and creates a pie chart  
    **Note:** This script uses relative paths and should be run from `/src`

- `/logs`
  - `log.csv`: Initial version
  - `log1.csv`: Version with partner check
  - `log2.csv`: Uses `Isend/Irecv` instead of `sendrecv`
  - `log3.csv`: Post-debug version

- `/draws`
  - Diagrams explaining the different AllReduce algorithm implementations
