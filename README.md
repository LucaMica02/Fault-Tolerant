# 🛡️ Fault-Tolerant Allreduce 

[![MPI Version](https://img.shields.io/badge/OpenMPI-5.0.x-blue)](https://www.open-mpi.org/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Singularity](https://img.shields.io/badge/Singularity-3.0%2B-orange)](https://sylabs.io/docs/)

## 📘 Introduction
The goal of this repository is to extend the implementation of the **Recursive Doubling** and **Rabenseifner** Allreduce algorithms in **OpenMPI**, making them **fault-tolerant** — that is, ensuring they can continue working correctly even after one or more process failures.

This work builds upon **ULFM (User-Level Failure Mitigation)**, an MPI extension providing mechanisms for failure detection and recovery.

---

## ⚙️ Prerequisites

We rely on **ULFM**, integrated in the community release of **OpenMPI** starting from version **5.0.x**.  
For more details about ULFM, see the [ULFM project website](https://fault-tolerance.org/).

If your MPI installation does not support ULFM, you can either:
- Install a newer version of OpenMPI with ULFM, or  
- Use a **containerized environment** (recommended for HPC clusters).  
  We use **Singularity**, as it’s widely supported in HPC systems.

---

### 🔍 Check ULFM Support

```bash
cd Fault-Tolerant/sanity
make
mpirun --with-ft=ulfm -n <np> ./ulfm.exe
```

If it compiles and runs successfully, ULFM is supported.
Otherwise, you can use a Singularity container as shown below

```bash
mkdir -p $HOME/local && cd $HOME/local
singularity build mpi-ft-ulfm.sif docker://abouteiller/mpi-ft-ulfm:latest

mkdir -p $HOME/tmp
echo 'export TMPDIR=$HOME/tmp' >> ~/.bashrc
source ~/.bashrc
```

Now, try again:

```bash
cd Fault-Tolerant/sanity
singularity exec -B $HOME/local $HOME/local/mpi-ft-ulfm.sif make
singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR \
$HOME/local/mpi-ft-ulfm.sif mpirun --with-ft ulfm -n <np> ./ulfm.exe
```

## 🧩 HPC Environment Checks

It is recommended to navigate to the `/sanity` folder and run the `mpi_check.slurm` and `ulfm_check.slurm` scripts.  
These scripts help verify that your environment is correctly set up for both **MPI** and **ULFM** execution.

---

## 🏗️ Project Structure
- **slurm/** – batch job scripts for experiments on HPC systems  
- **run/** – orchestration scripts for running and managing tests  
- **analysis/** – data processing and visualization utilities  
- **sanity/** – simple MPI/ULFM tests to ensure the environment is set up correctly  
- **src/** – source code for both original and fault-tolerant implementations  
- **data/** – experimental results and datasets 

---

## 🧪 Testing and Validation

If your environment supports **ULFM**, all the testing scripts are configured to run using **Singularity**.  
If you want to run the tests directly with **OpenMPI**, you may need to modify the scripts accordingly, or follow the prerequisite steps to install Singularity.

The scripts in the `/slurm` folder were used to collect the data currently available in the `/data` folder.

### Running a Simple Test

1. **Compile the source code**  
Navigate to the `src/rd` and `src/rabena` folders and compile the code using Singularity:
```bash
singularity exec -B $HOME/local -B $TMPDIR:$TMPDIR $HOME/local/mpi-ft-ulfm.sif make
```

2. **Navigate to the /run folder and run a test**  
```bash
cd /run
./run_test.sh <kill_value> <log_file.csv> <algorithm> <path_to_executable>
```
- kill_value: 0 = no kill, 1 = single kill, 2 = multiple kills
- log_file.csv: path to save the log file
- algorithm: rd (Recursive Doubling) or raben (Rabenseifner)
- path_to_executable: e.g., ../src/rd/main or ../src/raben/main

Example: ```bash./run_test.sh 0 ../log/sample.csv rd ../src/rd/main```

3. **Check the output log**  
```bash
<float expected> <int expected>
[NP, DELAY (s), BUFFER SIZE, KILL, TIME TAKEN (s), DEADLOCK, SEGFAULT, ABORT, RIGHT RESULT]
```
The data will also be saved in the specified log file.

4. **Analyze the results**  
You can use the analysis script under /analysis to generate reports:
```bash
cd analysis
python3 analyze_log.py
```
This provides a summary regarding deadlocks, aborts, correct results, and recovery success.

### Plotting the Data

To visualize performance and fault-tolerance results, you can run the following scripts:

- **`analysis/analyze_compare.py`**  
  Shows the runtime difference between the original and custom algorithm across different message sizes and numbers of processes.  
  *Focuses on the overhead when no faults occur.*

- **`analysis/analyze_fault.py`**  
  Shows data from tests with no faults and with a single fault.  
  *Focuses on the overhead introduced when a failure occurs.*

Tests were executed on the **Leonardo supercomputer**, and the workflow is designed to function in any **SLURM-based HPC cluster**.  
Collected results in the `/data` directory represent performance and reliability experiments conducted on Leonardo.

---

## ⚠️ Limitations and Considerations

The current implementation focuses on **fault awareness** during Allreduce operations.  
When one or more processes fail during communication:
- The algorithm detects the failure using ULFM.
- Depending on the communication stage, it decides whether recovery is possible.
- In some cases, the operation can resume and complete successfully; in others, it must safely abort.

This approach aims to minimize data loss and communication deadlocks, though complete recovery cannot always be guaranteed.

---

## 🚀 Future Work

Planned future extensions include:
- Expanding fault-tolerant support to additional Allreduce algorithms.
- Generalizing the framework to other collective communication patterns.
- Integrating fault-tolerant Allreduce into real-world HPC applications.
- Performing large-scale evaluations across different cluster architectures.

---

## 🙏 Acknowledgments

This project forms the foundation of my **Bachelor’s Thesis** in Computer Science at **Sapienza University of Rome**.  
For questions or collaboration, feel free to reach out:

📧 *micarelli.2061752@studenti.uniroma1.it*

This work builds on the **ULFM (User-Level Failure Mitigation)** framework for MPI fault tolerance.  
Special thanks to the ULFM authors:

> Wesley Bland, Aurelien Bouteiller, Thomas Herault, George Bosilca, Jack J. Dongarra  
> *Post-failure recovery of MPI communication capability: Design and rationale.*  
> *International Journal of High Performance Computing Applications, 27(3): 244–254 (2013)*