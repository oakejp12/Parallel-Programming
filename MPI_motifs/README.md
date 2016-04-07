Compile serial program using *g++ -g -Wall -o main main.cpp -std=c++0x*
Compile MPI program using *mpicxx -g -Wall mpi_motifs.cpp -o mpi_motifs -std=c++0x*
Run the MPI program using *mpiexec -n <number of processes> ./mpi_motifs motifsSmall.txt sequencesSmall.txt output.txt*
