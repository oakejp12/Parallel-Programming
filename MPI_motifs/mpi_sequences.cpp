/*
* Johan Oakes - Program 4 - Part 2 - March 31, 2016
*/
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <mpi.h>
#include "etime.h"

using namespace std;

int main(int argc, char* argv[]) {
  // Processor O variables
  int M, K_M, S, K_S; // Vars from file input
  int* globalCount; // Rank 0 combines all the local counters to globalCount

  // Processor 0+ variables
  int motif_length; // Helper variable to make code more readable - more descriptive
  int flag = 1; // Tell whether a match happens or not
  char* motifs; // Hold the motifs from file input
  char* sequences; // Hold the global sequences from file input
  char* local_sequences; // Local: Each processor gets their portion of sequences to deal with
  int* counter; // Local: Count instances of matches

  // MPI Initialization
  int rank, size;
  MPI_Init(nullptr, nullptr);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank of the process this thread is running on
  MPI_Comm_size(MPI_COMM_WORLD, &size); // Get the number of processors this job is using

  /*
  * Add the null terminating string
  * If there exists more than K_M char's then the extra chars & null termination
  *  will be written outside the end of the array, overwriting memory not belonging
  *  to the array
  * Calloc allocates and initializes to all zeros (like np.zeros in Python)
  */


  // Processor 0 does all the I/O & distribution of the data
  if (rank == 0) {
    // Read in the values from the file
    ifstream in_motifs(argv[1]);
    ifstream in_sequences(argv[2]); // Sequences file
    if (!in_motifs.is_open()) {
      cout << "System could not open file!" << endl;
      return EXIT_FAILURE;
    } else {
      // Read motifs file here
      in_motifs >> M >> K_M;

      // Declare motifs array to read in values
      motifs = (char *)calloc(((M*K_M)+sizeof(char)), sizeof(char));
      int mo_len;
      for (int i = 0; i < M; i++) {
        mo_len = int(strlen(motifs));
        in_motifs >> (motifs + mo_len);
      }
    }
    if (!in_sequences.is_open()) {
      cout << "System could not read sequences file!" << endl;
      return EXIT_FAILURE;
    } else {
      // Read sequences file here
      in_sequences >> S >> K_S;

      // Exit immediately if lengths do not match
      if (K_M != K_S) {
        cout << "Motif character length do not match!" << endl;
        return EXIT_FAILURE;
      } else {
        motif_length = K_M;
      }

      // Create sequences array to read in values
      sequences = (char *)calloc((S*motif_length)+sizeof(char), sizeof(char));
      int seq_len;
      for (int i = 0; i < S; i++) {
        seq_len = int(strlen(sequences));
        in_sequences >> (sequences + seq_len);
      }
    }
    // Close the files
    in_motifs.close();
    in_sequences.close();

    // Initialize a global counter
    globalCount = (int *)calloc(size * M + 1, sizeof(int));

    tic();
  }

  MPI_Barrier(MPI_COMM_WORLD);

  /*
  * Broadcast: number of motifs
  *            motif lengths
  *            sequence lengths
  */
  MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&motif_length, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&S, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // Since no other processes have allocated it yet
  if (rank != 0) {
    motifs = (char *)calloc((M*motif_length)+sizeof(char), sizeof(char));
  }

  // Declare buffers to use in algorithm
  local_sequences = (char *)calloc(((S/size) * motif_length + sizeof(char)), sizeof(char));
  counter = (int *)calloc(M, sizeof(int));

  // Broadcast the sequences array
  MPI_Bcast(motifs, (M * motif_length) + sizeof(char), MPI_CHAR, 0, MPI_COMM_WORLD);

  // Each processor gets their portion of the local motifs
  MPI_Scatter(sequences, (S/size) * motif_length, MPI_CHAR, local_sequences, (S/size) * motif_length, MPI_CHAR, 0, MPI_COMM_WORLD);

  // For each sequence
  for (int i = 0; i < (S/size); i++) {
    // For each motif
    for (int j = 0; j < M; j++) {
      // Check that each character matches
      // When no match exists, turn off and break
      for (int m = 0; m < motif_length; m++) {
        // Make the code more readable
        // Characters are matched against each other
        char motif_match = (local_sequences[i * motif_length + m] != motifs[j * motif_length + m]);
        char X_match = (motifs[j * motif_length + m] != 'X' );
        if (motif_match && X_match) {
          flag = 0;
          break;
        }
      }
      // If there was a match that motif deserves another vote!
      if (flag == 1) {
        counter[j]++;
      } else {  // No match :( -> no vote
        flag = 1;
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // Gather results!
  MPI_Gather(counter, M, MPI_INT, globalCount, M, MPI_INT, 0, MPI_COMM_WORLD);

  // Since motifs correspond modular wise, find like motifs and increment their count
  // Position 1 and Position 6 will hold the count for the same motif of length 5
  for (int i = M+2; i < (size * M); i++) {
    globalCount[i % M] += globalCount[i];
  }

  // Check MPI Results
  MPI_Barrier(MPI_COMM_WORLD);

  // Results ---- ???????????????

  if (rank == 0) {
    toc();

    cout << "Elapsed time: " << etime() << endl;

    ofstream output(argv[3]);
    //Output to ofstream file
    int index = 0;
    int length = int(strlen(motifs));
    output << length / motif_length << endl;
    for (int i = 0; i < (length - 1); i++) {
      if (i % motif_length == 0 && i != 0) {
        output << "," << globalCount[index] << endl;
        index++;
      }
      if (i != length) {
        output << motifs[i];
      }
    }

    output.close();
  }

  MPI_Barrier(MPI_COMM_WORLD);


  free(local_sequences);
  free(counter);

  if (rank == 0) {
    free(motifs);
    free(sequences);
    free(globalCount);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
