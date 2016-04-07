#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>

#include "etime.h"

using namespace std;

int main(int argc, char* argv[]) {
  cout << "Starting main program..." << endl;
  /**
  * Format of file
  * M motifs - K chars/motifs
  * M rows of K character long strings
  */

  if (argc != 4) {
    cout << "In correct number of arguments!" << endl;
    return EXIT_FAILURE;
  } else {

    /* Read in the values from the file */

    // Variables from file
    int M, K_M, S, K_S;
    vector<string> motifs;
    vector<string> sequences;

    ifstream in_motifs (argv[1]);

    if (!in_motifs.is_open()) {
      cout << "System could not open file!" << endl;
      return EXIT_FAILURE;
    } else {

      /* Read motifs file here */

      in_motifs >> M >> K_M;
      cout << "There are " << M << " motifs, each of length " << K_M << "!" << endl;

      string x; // Placeholder char
      while(in_motifs >> x)
      motifs.push_back(x);

      // print_vec(motifs, "motifs");
    }

    ifstream inSequences (argv[2]);

    if (!inSequences.is_open()) {
      cout << "System could not read sequences file!" << endl;
      return EXIT_FAILURE;
    } else {

      /* Read sequences file here */

      inSequences >> S >> K_S;
      cout << "There are " << S << " sequences of length " << K_S << "!" << endl;

      string x;
      while(inSequences >> x)
      sequences.push_back(x);

    }

    if (K_S != K_M) {
      cout << "Motif character lengths do not match!" << endl;
      return EXIT_FAILURE;
    } else {

      /* Start the main program here */
      cout << "Starting motif matching against sequences..." << endl;

      /*
      * To search for a motif in a set of sequences:
      * Compare the char's in the matching positions between the motif and given sequence
      *
      */

      int seq_size = sequences.size(), motif_size = motifs.size(), flag = 0;
      unordered_map<string, int> matches; // Keep a hash of matches with appearance count

      tic();
      // For each motif
      for (int i = 0; i < motif_size; i++) {
        // For each sequence
        for (int j = 0; j < seq_size; j++) {

          /* Match against sequences */

          // Hold current motif size so we don't
          // have to perform a look up each time
          int curr_motif_sz = motifs[i].length();

          // Match character by character
          for (int x = 0; x < curr_motif_sz; x++) {
            // Used to make the code more readable
            char X_match = (motifs[i][x] == 'X');
            char motif_match = (motifs[i][x] == sequences[j][x]);

            if (motif_match || X_match) {} // Characters matched!
            else {
              flag = 0;
              break; // We can exit since sequence won't match motif
            }

            flag = 1; // All character's matched
          }

          if (flag == 1) {
            // We found a motif match booooy
            matches[motifs[i]]++;
          }
        }
      }
      toc();

      // Prepare output file
      ofstream output(argv[3]);
      output << matches.size() << endl;
      for (auto x : matches){
        output << x.first << ", " << x.second << endl;
      }

      // Close output file
      output.close();
    }

    // Close the files
    in_motifs.close();
;    inSequences.close();
  }

  cout << "Elapsed time: " << etime() << endl;
  cout << "Ending main program!" << endl;
  return EXIT_SUCCESS;
}
