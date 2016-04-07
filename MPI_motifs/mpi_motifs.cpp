/*
* Johan Oakes - Program 4 - Part 2 - March 31, 2016
*/
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include "etime.h"

using namespace std;

int main(int argc, char* argv[]) {

  // Processor O variables
  int M, K_M, S, K_S;
  int* globalCount; // Holds the global count for each motif

  // Processor 0+ variables
  int motif_length; // Helper variable to get rid of K_M
  int flag = 1; // Tell whether a match happens or not
  char* motifs; // Hold the whole motifs file
  char* sequences; // Hold the whole sequences file
  char* local_motifs; // Local: each processors portions of the motifs file
  char* matches; // Local : hold matches when they exist
  int* counter; // Local : portion of motif count

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
      for (int i = 0; i < M; i++) {
        in_motifs >> (motifs + strlen(motifs));
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
      for (int i = 0; i < S; i++) {
        in_sequences >> (sequences + strlen(sequences));
      }
    }
    // Close the files
    in_motifs.close();
    in_sequences.close();

    cout << "Values for initilization motifs:  " << M << " length: " << K_M << endl;
    cout << "Values for initilization sequences:  " << S << " length: " << K_S << endl;

    // Initialize a global counter
    int correct_sz = int(strlen(motifs))/motif_length;
    globalCount = (int *)calloc(correct_sz + 1, sizeof(int));
  }

  /*
  * Structure of parallel program: distributing motifs!!
  * 1. Process with rank 0 - I/O and distribution of data - including it's own
  * 2. Each processor gets complete copy of the motifs - MPI_Bcast
  * 3. Pprocessors != 0 should just do the counting
  * 4. Number of motifs will always be a multiple of the number of processors being employed
  * 5. Exploit MPI collective communication functions
  */

  // TODO: Start timing here
  if (rank == 0 ) {
    tic();
  }

  /*
  * Broadcast: number of motifs
  *            motif lengths
  *            sequence lengths
  */
  MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&motif_length, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&S, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // Since no other processes have allocated memory for it yet
  if (rank != 0) {
    sequences = (char *)calloc((S*motif_length)+sizeof(char), sizeof(char));
  }

  // Declare buffers to use in algorithm
  local_motifs = (char *)calloc(((M/size) * motif_length + sizeof(char)), sizeof(char));
  matches = (char *)calloc(((M/size)* motif_length + sizeof(char)), sizeof(char));
  counter = (int *)calloc((M/size), sizeof(int));

  // Broadcast the sequences array
  MPI_Bcast(sequences, (S*motif_length)+sizeof(char), MPI_CHAR, 0, MPI_COMM_WORLD);

  // Each processor gets their portion of the local motifs
  MPI_Scatter(motifs, (M/size) * motif_length, MPI_CHAR, local_motifs, (M/size) * motif_length, MPI_CHAR, 0, MPI_COMM_WORLD);

  int matched_index = 0;
  int counter_index = 0;
  // For each motif
  for (int i = 0; i < (M/size); i++) {

    // Add motif to the MccGyver created hash map
    for (int m = 0; m < motif_length; m++) {
        matches[matched_index] = local_motifs[i * motif_length + m];
        matched_index++;
    }

    // For each sequence
    for (int j = 0; j < S; j++) {
      // Match character by character
      for (int x = 0; x < motif_length; x++) {

        // Refactored to make the code more readable
        char X_match = (local_motifs[i * motif_length + x] != 'X');
        char motif_match = (local_motifs[i * motif_length + x] != sequences[j* motif_length + x]);

        // Match against each character
        if (motif_match && X_match) {
          flag = 0;
          break;
        }
      }

      if (flag == 1){
        // cout << "It's a match!" << endl;
        counter[counter_index]++;
      } else {
        flag = 1;
      }
    }
    counter_index++;
  }

  // Rewrite the results into motifs so null the motif array
  // We don't need it
  if (rank == 0) {
    memset(motifs, '\0', sizeof(char)*((M*motif_length)+sizeof(char)));
  }

  // Gather results!
  MPI_Gather(matches, (M/size)*motif_length, MPI_CHAR, motifs, (M/size)*motif_length, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Gather(counter, (M/size), MPI_INT, globalCount, (M/size), MPI_INT, 0, MPI_COMM_WORLD);

  // Results ---- ???????????????

  if (rank == 0) {
    toc();

    cout << "Elapsed time: " << etime() << endl;

    ofstream output(argv[3]);
    //Output to ofstream file
		int index = 0;
    int length = int(strlen(motifs));
		output << length / motif_length << endl;
		for (int i = 0; i < (length); i++) {
			if (i % motif_length == 0 && i != 0) {
				output << "," << globalCount[index] << endl;
				index++;
			}
			if (i != int(strlen(motifs))) {
				output << motifs[i];
			}
		}

    output.close();
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // Free the memory held up by each process
  free(local_motifs);
  free(matches);
  free(counter);

  if (rank == 0) {
    free(motifs);
    free(sequences);
    free(globalCount);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
