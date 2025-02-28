#!/bin/bash
#SBATCH --job-name=omp_mpi
#SBATCH --time=03:00:00
#SBATCH --nodes=2 --ntasks-per-node=8
#SBATCH --mem=8gb

module load intel/icc18
module load intel/mpi4


gcc -fopenmp -o omp_2 lab2_omp.c
mpicc -o mpi_2 lab2_mpi.c


export I_MPI_LIBRARY=/usr/lib64/slurm/mpi_pmi2.so
./omp_2 4 1
./omp_2 8 1
./omp_2 16 1


srun --mpi=pmi2 -n 4 ./mpi_2 1
srun --mpi=pmi2 -n 8 ./mpi_2 1