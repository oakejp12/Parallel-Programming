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
  int flag = 1;
  int thread_count = atoi(argv[1]);
  vector<string> motifs;
  vector<string> sequences;
  unordered_map<string,int> result;


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
      cout << "System could not open motifs file!" << '\n';
      exit(4);
    }

    else {

      // Read motifs file here
      in_motifs >> M >> K_M;

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
  * Sequences should be split evenly among processors
  * Number of sequences will always be a multiple of the number of processors being employed
  * Use the default iteration scheduler
  */

  tic();

  #pragma omp parallel for num_threads(thread_count)
  for (int i = 0; i < S; i++) { // Sequence iteration
    for (int j = 0; j < M; j++) { // Motif iteration

      // So that each motif get's counted
      if (i == 0) {
        result[motifs[j]] = 0;
      }

      // Match character by character
      for (int x = 0; x < K_M; x++) {

        // Used to make the code more readable
        char X_match = (motifs[j][x] != 'X');
        char motif_match = (sequences[i][x] != motifs[j][x]);

        // If there is no match then break out of the loop
        if (motif_match && X_match) {
          flag = 0;
          break;
        }
      }

      // If a match did exist, add it to the hash map
      if (flag == 1) {
        // We found a motif match booooy
        result[motifs[j]]++;
      } else {
        flag = 1;
      }
    }
  }


  toc();

  #pragma omp master
  {
    ofstream output(argv[4]);
    output << result.size() << '\n';
    for (auto const& x : result) {
      output << x.first << ", " << x.second << '\n';
    }

    output.close();
  }

  cout << "Elapsed time: " << etime() << '\n';
  cout << "Ending main program!" << '\n';
  return 0;
}
