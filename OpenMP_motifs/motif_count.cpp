#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include "etime.h"

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;


// Prototype for motif_count
void motif_count(vector<string> motifs, vector<string> sequences, int numMotifs, unordered_map<string,int>& matches);


int main(int argc, char* argv[]) {
  /**
  * Format of file
  * M motifs - K chars/motifs
  * M rows of K character long strings
  */

  // Variables from file
  int M; // Number of motifs
  int K_M; // Length of motifs
  int S; // Number of sequences
  int K_S; // Length of sequences
  int thread_count = atoi(argv[1]);
  vector<string> motifs;
  unordered_map<string,int> matches;
  vector<string> sequences;

  if (argc != 5) {
    cout << "In correct number of arguments!" << '\n';
    exit(11);
  }

  // Master thread takes care of file I/O
  #pragma omp master
  {
    ifstream in_motifs (argv[2]);
    ifstream inSequences (argv[3]);

    if (!in_motifs.is_open()) {
      cout << "System could not open file!" << '\n';
      exit(4);
    }

    else {

      // Read motifs file here
      in_motifs >> M >> K_M;
      cout << "There are " << M << " motifs, each of length " << K_M << "!" << '\n';

      string x; // Placeholder char
      while(in_motifs >> x)
      motifs.push_back(x);
    }

    if (!inSequences.is_open()) {
      cout << "System could not read sequences file!" << '\n';
      exit(4);
    }

    else {
      // Read sequences file here
      inSequences >> S >> K_S;
      cout << "There are " << S << " sequences of length " << K_S << "!" << '\n';

      if (K_S != K_M) {
        cout << "Motif character lengths do not match!" << '\n';
        exit(1);
      }

      string x;
      while(inSequences >> x)
      sequences.push_back(x);
    }

    // Close the files
    in_motifs.close();
    inSequences.close();
  }

  /*
  * To search for a motif in a set of sequences:
  * Compare the char's in the matching positions between the motif and given sequence
  * Motifs should be split evenly among processors
  * Number of motifs will always be a multiple of the number of processors being employed
  * Use the default iteration scheduler
  */

  tic();

  #pragma omp parallel num_threads(thread_count)
  motif_count(motifs, sequences, M, matches);

  toc();

  #pragma omp master
  {
      ofstream output(argv[4]);
      output << matches.size() << '\n';
      for (auto x : matches) {
        output << x.first << ", " << x.second << '\n';
      }

      output.close();
  }

  cout << "Elapsed time: " << etime() << '\n';
  cout << "Ending main program!" << '\n';
  return 0;
}


void motif_count(vector<string> motifs, vector<string> sequences, int numMotifs, unordered_map<string, int>& matches) {

  int rank = omp_get_thread_num();
  int thread_count = omp_get_num_threads();
  int range = numMotifs / thread_count;

  vector<string>::iterator first = (motifs.begin() + (rank*range));
  vector<string>::iterator last = first + range;
  vector<string> local_motifs (first, last);

  unordered_map<string,int> local_result;
  int seq_size = sequences.size();
  int mot_size = local_motifs.size();
  int flag = 1;

  // For each motif
  for (int i = 0; i < mot_size; i++) {

    // Add the current string to our hash map
    // Set its value to 0 since there haven't been any matches
    local_result[local_motifs[i]] = 0;

    // For each sequence
    for (int j = 0; j < seq_size; j++) {


      // Hold current motif size so we don't
      // have to perform a look up each time
      int curr_motif_sz = local_motifs[i].length();

      // Match character by character
      for (int x = 0; x < curr_motif_sz; x++) {
        // Used to make the code more readable
        char X_match = (local_motifs[i][x] != 'X');
        char motif_match = (local_motifs[i][x] != sequences[j][x]);

        // If there is not match then break out of the loop
        if (motif_match && X_match) {
          flag = 0;
          break;
        }
      }

      // If a match did exist, add it to the hash map
      if (flag == 1) {
        // We found a motif match booooy
        local_result[local_motifs[i]]++;
      } else {
        flag = 1;
      }
    }
  }

  // Add the local hash map of result to the global hash map
  #pragma omp critical
  matches.insert(local_result.begin(), local_result.end());
}
