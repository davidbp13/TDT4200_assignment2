#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include "bitmap.h"

#define XSIZE 2560 // Size of before image
#define YSIZE 2048

int main(int argc, char** argv) {
	MPI_Init(NULL, NULL); // MPI Initialization
	
	// Get the id of each process and the total amount of processes
	int my_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int num_proc;
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	
	// Check if the number of processes is valid
	if (YSIZE % num_proc != 0){
		// Only process 0 will print the error
		if (my_rank == 0) {
			printf("ERROR: the number of processes is not a multiple of %d\n", YSIZE); 
		}
		
		// All processes finalize MPI
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
	
		return 0;
	}
	
	// Root proceess reads the image
	int image_size = XSIZE * YSIZE * 3;
	uchar *image = NULL;
	if (my_rank == 0) {
		image = (uchar *)malloc(sizeof(uchar) * image_size);
		assert(image != NULL);
		readbmp("before.bmp", image);
		//printf("Process %d read the image succesfully\n", my_rank);
	}
	
	// For each process, create a buffer that will hold a subset of the entire image
	int image_chunk_size = (YSIZE / num_proc) * XSIZE * 3;
	uchar *subset = (uchar *)malloc(sizeof(uchar) * image_chunk_size);
	assert(subset != NULL);
	//printf("Process %d allocated chunk size of %d succesfully\n", my_rank, image_chunk_size);

	// Scatter image between the processes
	MPI_Scatter(image, 				// Data on root process
				image_chunk_size, 	// Number of elements sent to each process
				MPI_UNSIGNED_CHAR, 	// Data type
				subset,				// Where to place the scattered data
				image_chunk_size, 	// Number of elements to receive
				MPI_UNSIGNED_CHAR,  // Data type
				0, 					// Rank of root process
				MPI_COMM_WORLD);	// MPI communicator
	//printf("Process %d scattered succesfully\n", my_rank);
	
	// Invert colors in each of the subsets of the image
	for (int i=0; i<(YSIZE / num_proc); i++ ) { // To iterate through rows
		for (int j=0; j<XSIZE*3; j+=3) { // To iterate through columns (and each pixel component)
			subset[3 * i * XSIZE + j + 0] = 255 - subset[3 * i * XSIZE + j + 0]; 
			subset[3 * i * XSIZE + j + 1] = 255 - subset[3 * i * XSIZE + j + 1]; 
			subset[3 * i * XSIZE + j + 2] = 255 - subset[3 * i * XSIZE + j + 2]; 
		}
	}
	
	// Gather the results from the processes
	uchar *output_image = NULL;
	if (my_rank == 0) {
		output_image = (uchar *)malloc(sizeof(uchar) * image_size);
		assert(output_image != NULL);
	}
	MPI_Gather(subset, 				// Data to gather
				image_chunk_size, 	// Number of elements of each gathered data
				MPI_UNSIGNED_CHAR, 	// Data type
				output_image, 		// Where to place gathered data
				image_chunk_size, 	// Number of elements recevied per process
				MPI_UNSIGNED_CHAR,  // Data type
				0, 					// Rank of root process
				MPI_COMM_WORLD);	// MPI communicator
	//printf("Process %d gathered succesfully\n", my_rank);
	
	// Root proceess saves the image
	if (my_rank == 0) {
		savebmp("after.bmp", output_image, XSIZE, YSIZE);
	}
		
	// Clean up
	if (my_rank == 0) {
		free(output_image);
		free(image);
	}
	free(subset);
	
	// Finalize MPI
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	
	return 0;
}
