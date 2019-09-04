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
	int image_chunk_size = ((YSIZE / num_proc) * XSIZE * 3) + (XSIZE * 3); // New chunk should be bigger (should be able to hold one more row)
	uchar *subset = (uchar *)malloc(sizeof(uchar) * image_chunk_size);
	assert(subset != NULL);
	//printf("Process %d allocated subset of size chunk size of %d succesfully\n", my_rank, image_chunk_size);

	// Calculate send counts and displacements
	int *sendcounts = malloc(sizeof(int)*num_proc); // Array describing how many elements to send to each process
    int *displs = malloc(sizeof(int)*num_proc);     // Array describing the displacements where each segment begins
    int rem = YSIZE % num_proc; 					// Rows remaining after division among processes
    int sum = 0;                					// Sum of counts. Used to calculate displacements
    
    
	for (int i = 0; i < num_proc; i++) {
        sendcounts[i] = YSIZE / num_proc; // Number of rows per process
        if (rem > 0) {
            sendcounts[i] += 1; // If there are remaining rows, distribute them among the processes
            rem--;
        }
		
		sendcounts[i] = sendcounts[i] * XSIZE * 3; // Convert rows into image buffer elements
        displs[i] = sum;
        sum += sendcounts[i];
        //printf("sendcounts(%d) = %d and displs(%d) = %d\n", i, sendcounts[i], i, displs[i]);
    }
    
    
	// Scatter image between the processes
	MPI_Scatterv(image, 				// Data on root process
				 sendcounts, 			// Array with the number of elements sent to each process
				 displs, 				// Displacement relative to the image buffer
				 MPI_UNSIGNED_CHAR, 	// Data type
				 subset,				// Where to place the scattered data
				 image_chunk_size, 		// Number of elements to receive
				 MPI_UNSIGNED_CHAR,  	// Data type
				 0, 					// Rank of root process
				 MPI_COMM_WORLD);		// MPI communicator
	//printf("Process %d scattered succesfully\n", my_rank);
	
	// Invert colors in a checkered pattern of the subsets of the image
	int row_number = (YSIZE / num_proc) * (displs[my_rank] / sendcounts[my_rank]);
	//printf("displs = %d, sendcounts = %d, row number = %d\n", displs[my_rank], sendcounts[my_rank], row_number);
	for (int i=0; i<(sendcounts[my_rank] / (XSIZE*3)); i++ ) { // To iterate through rows
		for (int j=0; j<XSIZE*3; j+=6) { // To iterate through columns (and each pixel component)
			if (row_number % 2 == 0){
				subset[3 * i * XSIZE + j + 0] = 255 - subset[3 * i * XSIZE + j + 0]; 
				subset[3 * i * XSIZE + j + 1] = 255 - subset[3 * i * XSIZE + j + 1]; 
				subset[3 * i * XSIZE + j + 2] = 255 - subset[3 * i * XSIZE + j + 2]; 
				subset[3 * i * XSIZE + j + 3] = subset[3 * i * XSIZE + j + 3]; 
				subset[3 * i * XSIZE + j + 4] = subset[3 * i * XSIZE + j + 4]; 
				subset[3 * i * XSIZE + j + 5] = subset[3 * i * XSIZE + j + 5]; 
			}
			else{
				subset[3 * i * XSIZE + j + 0] = subset[3 * i * XSIZE + j + 0]; 
				subset[3 * i * XSIZE + j + 1] = subset[3 * i * XSIZE + j + 1]; 
				subset[3 * i * XSIZE + j + 2] = subset[3 * i * XSIZE + j + 2]; 
				subset[3 * i * XSIZE + j + 3] = 255 - subset[3 * i * XSIZE + j + 3]; 
				subset[3 * i * XSIZE + j + 4] = 255 - subset[3 * i * XSIZE + j + 4]; 
				subset[3 * i * XSIZE + j + 5] = 255 - subset[3 * i * XSIZE + j + 5]; 
			}
		}
		row_number++;
	}
	
	// Gather the results from the processes
	uchar *output_image = NULL;
	if (my_rank == 0) {
		output_image = (uchar *)malloc(sizeof(uchar) * image_size);
		assert(output_image != NULL);
	}
	
	MPI_Gatherv(subset, 			// Data to gather
				sendcounts[my_rank],// Number of elements of each gathered data
				MPI_UNSIGNED_CHAR, 	// Data type
				output_image, 		// Where to place gathered data
				sendcounts, 		// Number of elements recevied per process
				displs,				// Displacement relative to the image buffer
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
	free(sendcounts);
    free(displs);
	
	// Finalize MPI
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	
	return 0;
}
