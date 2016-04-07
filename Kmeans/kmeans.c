#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "etime.h"


/* Global variables
* Set from lowest memory to highest memory
 */
int NUM_CLUSTERS, NUM_THREADS, samples, features,
modified = 1; // Modified kills the program when all clusters settled
int *cluster,  // Holds cluster membership
*means; // Used to check if clusters are still relocating
double **points, // Read in the data values from input file
**assoc; // Store coordinates of each cluster
pthread_mutex_t mutex;
pthread_barrier_t barrier;


/* Helper function to print out the points */
void print_points(double** values, int rows) {
  int i, j;
  for (i = 0; i < rows; i++) {
    for (j = 0; j < features; j++) {
      printf("%.2f ", values[i][j]);
    }
    printf("\n");
  }
}

/* Helper function to add two points together to determine cluster membership */
static inline void add_to_sum(double* sum, double* point) {
  int i;
  for (i = 0; i < features; i++) {
    sum[i] += point[i];
	}
}

/* Euclidean algorithm to find distance between two points */
static inline double euc_dist(double *a, double *b) {
  int i;
  double sum = 0;
  // This is just the squared Euclidean distance
  // Which has greater weight on far off objects
  for (i = 0; i < features; i++) {
    sum += ((a[i]-b[i]) * (a[i]-b[i]));
  }
  return sum;
}

/* Find the nearest cluster to me, man */
void find_nearest(long start, long end) {
	int i, j;
	double curr_dist, min_dist = INT_MAX; // Minimum distance used to compute Euclidean
	for (i = start; i < end; i++) { //Per assigned samples (ie, 2 samples)
		pthread_mutex_lock(&mutex); // Only one thread can work on cluster membership
		min_dist = INT_MAX; // Set the Minimum dist to INT_MAX so we could closer objects
		for (j = 0; j < NUM_CLUSTERS; j++) { // Per cluster
			curr_dist = euc_dist(assoc[j], points[i]); // Check Euclidean distance between two points
			if (curr_dist < min_dist) { // Update minimum distance
				min_dist = curr_dist;
				cluster[i] = j; // Add to the cluster
			}
		}
		if (cluster[i] != means[i]) { // Check to see if clusters are still relocating
			means[i] = cluster[i];
			modified = 0;
		}

		// printf("Sample %i closest cluster: %i\n", (i), cluster[i]);
		pthread_mutex_unlock(&mutex);
	}
}

/* Calculate the means so that the center of each cluster
 * incorporates the new membership
 */
void calc_means(long start, long end) {
		int i, j, y, group_size; // group size of each cluster determines average
		for (i = 0; i < NUM_CLUSTERS; i++) { //Per Cluster
			pthread_mutex_lock(&mutex); // Messing with cluster membership again
			group_size = 0;
			for (j = start; j <= end; j++) { //For samples in each cluster
					if (cluster[j] == i) { // Each point index is in cluster
						add_to_sum(assoc[cluster[j]], points[j]); // Add points values to sum of cluster total
						group_size++;
					}
			}
			for (y = 0; y < features; y++) {
				assoc[i][y] = assoc[i][y] / (double) group_size; // Normalize each clusters size
			}
			pthread_mutex_unlock(&mutex);
		}
}

/* Actual algorithm to kick of k-Means */
void *kmeans_algo(void* rank) {
	// Divide up work evenly
	long my_rank = (long) rank; // Thread ID
	long chunk_size = samples / NUM_THREADS;
	long start = my_rank * chunk_size;
	long end = (my_rank + 1) * chunk_size - 1;
	if (my_rank == (NUM_THREADS - 1))
		end = samples - 1;

	// For each sample, find its closest cluster
	find_nearest(start, end);

	/* BARRIER */
	int sync = pthread_barrier_wait(&barrier);
	if(sync != 0 && sync != PTHREAD_BARRIER_SERIAL_THREAD)
  {
        printf("GET OUT! SAVE YOURSELF!\n");
        exit(-1);
  }

	int i, j; // Placeholder values

	if (my_rank == 0) {
		for (i = 0; i < NUM_CLUSTERS; i++) {
			for (j = 0; j < features; j++) {
				assoc[i][j] = 0;
			}
		}
	}


	/* BARRIER */
	sync = pthread_barrier_wait(&barrier);
	if(sync != 0 && sync != PTHREAD_BARRIER_SERIAL_THREAD)
  {
  	printf("GET OUT! SAVE YOURSELF!\n");
    exit(-1);
  }

	// Calculate the means of each cluster so membership is better represented
	calc_means(start, end);

	/* BARRIER */
	sync = pthread_barrier_wait(&barrier);
	if(sync != 0 && sync != PTHREAD_BARRIER_SERIAL_THREAD)
  {
  	printf("BARRIERerror\n");
    exit(-1);
  }

	return NULL;
}


int main(int argc, char** argv) {

	/* Correct number of args */
  if (argc != 5) {
    printf("Incorrect number of arguments homie....");
    exit(1);
  }

  /* Shared Variables */
  NUM_CLUSTERS = atoi(argv[1]); // K number of clusters
  NUM_THREADS = atoi(argv[2]); // P number of processors
  const char* IFNAME = argv[3]; // Input filename
  const char* OFNAME = argv[4]; // Output file
	int i, j, z; // Placeholder values
	time_t t;
	srand((unsigned) time(&t)); // Initializes random number generator

  /* Private variables */
  FILE* ifp; // Input file
  FILE* ofp = fopen(OFNAME, "w+"); // Output file


  // Read in the file
  ifp = fopen(IFNAME, "r");
  if (ifp == NULL) {
      fprintf(stderr, "File %s not found.\n", IFNAME);
      exit(1);
  };

   // Reading in samples and features from input file
	fscanf(ifp, "%i", &samples);
	fscanf(ifp, "%i", &features);

	// Initializing all globals
  cluster = (int *)malloc(samples * sizeof(int));
  assoc = (double **)malloc(NUM_CLUSTERS * sizeof(double *));

	// Points for the read in values from file
	// Set means to zero to indicate clustering hasn't begun
	points = (double **)malloc(samples * sizeof(double *));
	means = (int *)malloc(samples * sizeof(int));
  for (i = 0; i < samples; i++) {
       points[i] = (double *)malloc(features * sizeof(double));
       means[i] = 0;
  }

  // Read in the values provided by the input file
  double read;
  	for (i = 0; i < samples; i++) {
    	for (j = 0; j < features; j++) {
    		fscanf(ifp, "%lf", &read);
    		points[i][j] = read;
    	}
    }

		// Creating cluster membership for comparison
		// First corrdinates randomize points for a random k-Means
    for (i = 0; i < NUM_CLUSTERS; i++)
         assoc[i] = (double *)malloc(sizeof(double) * features);
    for (i = 0; i < NUM_CLUSTERS; i++) {
			for (j = 0; j < features; j++) {
				assoc[i][j] = points[rand() % samples][j];
			}
		}

	/* Fire up the engines brah
	*  Thread and mutex handling
	*/
	long  pid;
  pthread_t* thread_handles= (pthread_t*) malloc (NUM_THREADS*sizeof(pthread_t));
  pthread_mutex_init(&mutex, NULL);
  pthread_barrier_init(&barrier, NULL, NUM_THREADS);


	for (z = 0; z < (NUM_CLUSTERS * 50); z++) {

    tic();
		/* Create the threads and push those boys out */
		for (pid = 0; pid < NUM_THREADS; pid++)
			pthread_create(&thread_handles[pid], NULL, kmeans_algo, (void*)pid);

		/* Bring back the men - they've done their work */
		for (pid = 0; pid < NUM_THREADS; pid++)
			pthread_join(thread_handles[pid], NULL);
    toc();

		// Check to see if we have finished clustering
		if (modified == 1) {
			break;
		} else {
			modified = 1;
		}
	}

	// Output file has got to learn
	fprintf(ofp, "%i\n", samples);
	for (i = 0; i < samples; i++) {
    fprintf(ofp, "%i\n", cluster[i]);
	}

  // How much time
  printf("Your program took %lf\n", etime());

	// Cleanup the memory
	pthread_mutex_destroy(&mutex);
	pthread_barrier_destroy(&barrier);

	int m;
	for (m = 0; m < samples; m++) {
		free(points[m]);
	}
	free(points);
	for (m = 0; m < NUM_CLUSTERS; m++) {
		free(assoc[m]);
	}
	free(assoc);
	free(cluster);
	free(means);



	return 0;
}
