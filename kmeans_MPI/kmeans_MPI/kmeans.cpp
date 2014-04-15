#include <cmath>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <mpi.h>

using namespace std;

typedef vector<double> Point;
typedef vector<Point> Points;

// Gives random number in range [0..max_value]
unsigned int UniformRandom(unsigned int max_value) {
    unsigned int rnd = ((static_cast<unsigned int>(rand()) % 32768) << 17) |
                       ((static_cast<unsigned int>(rand()) % 32768) << 2) |
                       rand() % 4;
    return ((max_value + 1 == 0) ? rnd : rnd % (max_value + 1));
}

double Distance(const double* points1, int pos1, const double* points2, int pos2, int D) {
    double distance_sqr = 0;
	//printf("Point ");
	for (int i = 0; i < D; ++i) {
		//printf(" %f %f", points1[pos1*D + i], points2[pos2*D + i]);
		distance_sqr += (points1[pos1*D + i] - points2[pos2*D + i]) * (points1[pos1*D + i] - points2[pos2*D + i]);
	}
	//printf(" %f \n", sqrt(distance_sqr));
    return sqrt(distance_sqr);
}

int FindNearestCentroid(const double* centroids, const double *data_local, int k, int K, int D) {
    double min_distance = Distance(data_local, k, centroids, 0, D);
    int centroid_index = 0;
    for (int i = 1; i < K; ++i) {
        double distance = Distance(data_local, k, centroids, i, D);
        if (distance < min_distance) {
            min_distance = distance;
            centroid_index = i;
        }
    }
    return centroid_index;
}

void PrintArray(const double* array, int K, int D) {
	for (int i = 0; i < K; ++i) {
		for (int d = 0; d < D; ++d) {
			printf(" %f", array[i*D + d]);
		}
		printf(" \n");
	}
}

// Calculates new centroid position as mean of positions of 3 random centroids
void GetRandomPosition(double* centroids, int i, int K, int D) {
    int c1 = rand() % K;
    int c2 = rand() % K;
    int c3 = rand() % K;
    for (int d = 0; d < D; ++d) {
        centroids[i*D + d] = (centroids[c1*D + d] + centroids[c2*D + d] + centroids[c3*D + d]) / 3;
    }
	printf("getrand");
	PrintArray(centroids, K, D);

}

/*vector<int> KMeans(double *_data, int K, int N, int D) {
    int data_size = N;
    int dimensions = D;
    vector<int> clusters(data_size);

    // Initialize centroids randomly at data points
    Points centroids(K);
	int *_centroids = (int*)malloc(K*D*sizeof(double));
	Points data;
	data.assign(N, Point(D));
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < D; j++)
			data[i][j] = _data[i*N + j];
	}

	//if (rank == 0) {
	{for (int i = 0; i < K; ++i) {
			centroids[i] = data[UniformRandom(data_size - 1)];
		}
	}
    
    bool converged = false;
    while (!converged) {
        converged = true;
        for (int i = 0; i < data_size; ++i) {
            int nearest_cluster = FindNearestCentroid(centroids, data_local, i, D);
            if (clusters[i] != nearest_cluster) {
                clusters[i] = nearest_cluster;
                converged = false;
            }
        }
        if (converged) {
            break;
        }

        vector<int> clusters_sizes(K);
        centroids.assign(K, Point(dimensions));
        for (int i = 0; i < data_size; ++i) {
            for (int d = 0; d < dimensions; ++d) {
                centroids[clusters[i]][d] += data[i][d];
            }
            ++clusters_sizes[clusters[i]];
        }
        for (int i = 0; i < K; ++i) {
            if (clusters_sizes[i] != 0) {
                for (int d = 0; d < dimensions; ++d) {
                    centroids[i][d] /= clusters_sizes[i];
                }
            } else {
                centroids[i] = GetRandomPosition(centroids);
            }
        }
    }

    return clusters;
}*/

void WriteOutput(const int* clusters, ofstream& output, int N) {
    for (int i = 0; i < N; ++i) {
        output << clusters[i] << endl;
    }
}

int main(int argc , char** argv) {

	int rank, commsize, len;
	char host[MPI_MAX_PROCESSOR_NAME];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //rank of current processor
	MPI_Comm_size(MPI_COMM_WORLD, &commsize); //amount of all processors
	MPI_Get_processor_name(host, &len); //get hostname

	int K, N, D;
	double* data;
	ofstream output;

	printf("1 %d\n", rank);

	if (rank == 0) {
		if (argc != 4) {
			std::printf("Usage: %s number_of_clusters input_file output_file\n", argv[0]);
			return 1;
		}

		K = atoi(argv[1]);

		char* input_file = argv[2];
		ifstream input;
		input.open(input_file, ifstream::in);
		if (!input) {
			cerr << "Error: input file could not be opened" << endl;
			return 1;
		}

		input >> N >> D;
		data = (double*)malloc(N * D *sizeof(double));
		for (int i = 0; i < N; ++i) {
			for (int d = 0; d < D; ++d) {
				double coord;
				input >> coord;
				data[i*D + d] = coord;
			}
		}
		input.close();

		printf("2 %d\n", rank);

		char* output_file = argv[3];
		output.open(output_file, ifstream::out);
		if (!output) {
			cerr << "Error: output file could not be opened" << endl;
			return 1;
		}
	}

	//Checking if N cannot be divided by commsize
	if (N % commsize != 0)
		MPI_Abort(MPI_COMM_WORLD, 1);
	int partsize = N / commsize;
	double *data_local = (double*)malloc(partsize*D*sizeof(double));
	int* clusters_local = (int*)malloc(partsize*sizeof(int));

	printf("3 %d\n", rank);

	// Broadcasting variables
	MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&D, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(data, partsize*D, MPI_DOUBLE,
		data_local, partsize*D, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	/*for (int i = 0; i < partsize; ++i) {
		for (int d = 0; d < D; ++d) {
			cout << data_local[i*D + d] << " ";
		}
		std::cout << endl;
	}*/
		
    srand(123); // for reproducible results
	
	// Initialize centroids randomly at data points
	double* centroids = (double*)malloc(K*D*sizeof(double));

	// master is choosing first centroids 
	if (rank == 0) {
		for (int i = 0; i < K; ++i) {
			unsigned int index = UniformRandom(N - 1);
			for (int d = 0; d < D; ++d) {
				centroids[i*D + d] = data[index*D + d];
				//printf(" %f", centroids[i*D + d]);
			}
			//printf(" \n");
		}
	}
	
	MPI_Bcast(centroids, K*D, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	bool converged = false;
	int* clusters_sizes = (int*)malloc(K*sizeof(int));
	printf("5 %d\n", rank);
	int counter = 0;
	while (!converged) {
		++counter;
		if (counter > 6) {
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		converged = true;
		for (int i = 0; i < partsize; ++i) {
			int nearest_cluster = FindNearestCentroid(centroids, data_local, i, K, D);
			//printf("%d\n", nearest_cluster);
			if (clusters_local[i] != nearest_cluster) {
				clusters_local[i] = nearest_cluster;
				//printf(" %d", clusters_local[i]);
				converged = false;
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);		
		if (converged) {
			MPI_Bcast(&converged, 1, MPI_C_BOOL, rank, MPI_COMM_WORLD);
		}
		MPI_Barrier(MPI_COMM_WORLD);

		if (converged) {
			break;
		}		

		for (int i = 0; i < partsize; ++i) {
			for (int d = 0; d < D; ++d) {
				centroids[clusters_local[i]*D + d] += data[i * D + d];
			}
			++clusters_sizes[clusters_local[i]];
		}

		int* clusters_sizes_glob = (int*)malloc(K*sizeof(int));
		double* centroids_glob = (double*)malloc(K*D*sizeof(double));

		for (int i = 0; i < K; ++i) {
			clusters_sizes_glob[i] = 0;
			for (int d = 0; d < D; ++d) {
				centroids_glob[i*D + d] = 0;
			}
		}
		
		//here broadcasting / reducing
		MPI_Allreduce(centroids, centroids_glob, K*D, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		MPI_Allreduce(clusters_sizes, clusters_sizes_glob, K, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

		for (int i = 0; i < K; ++i) {
			clusters_sizes[i] = clusters_sizes_glob[i];
			for (int d = 0; d < D; ++d) {
				centroids[i*D + d] = centroids_glob[i*D + d];
			}
		}
		
		//double* tmp = centroids; centroids = centroids_glob; centroids_glob = tmp;
		free(centroids_glob);
		//int* temp = clusters_sizes; clusters_sizes = clusters_sizes_glob; clusters_sizes_glob = temp;
		free(clusters_sizes_glob);

		printf("free");
		PrintArray(centroids, K, D);
		
		for (int i = 0; i < K; ++i) {
			if (clusters_sizes[i] != 0) {
				for (int d = 0; d < D; ++d) {
					//printf(" %d", clusters_sizes[i]);
					centroids[i*D + d] /= clusters_sizes[i];
				}
			}
			else {
				GetRandomPosition(centroids, i, K, D);
				printf("else");
				PrintArray(centroids, K, D);
				MPI_Bcast(centroids + i*sizeof(double), D, MPI_DOUBLE, rank, MPI_COMM_WORLD);
			}
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	int* clusters = (int*)malloc(N*sizeof(int));
	MPI_Gather(clusters_local, partsize, MPI_INT, clusters, partsize, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		WriteOutput(clusters, output, N);
		output.close();
	}

	free(data);
	free(data_local);
	free(clusters_local);
	free(clusters);
	free(centroids);
	//free(centroids_glob);
	free(clusters_sizes);
	//free(clusters_sizes_glob);

	MPI_Finalize();
    return 0;
}