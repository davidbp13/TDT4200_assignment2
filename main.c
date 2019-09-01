#include <stdlib.h>
#include <stdio.h>
#include <mpi.h> // To use MPI APIs
#include "bitmap.h"

#define XSIZE 2560 // Size of before image
#define YSIZE 2048

/*Idea:
 * Take a scatter/gather example as a base *
 * Process 0 will verify if the number of process is valid *
 * Process 0 will load the image *
 * Process 0 will use scatter to split the rows among the number of processes -
 * Each process will perform some kind of computation in their own row and return the finished row -
 * Process 0 will gather all the results and save the image -
*/

int main(int argc, char** argv) {
	MPI_Init(NULL, NULL); // MPI Initialization
	
	// Get the id of each process and the total amount of processes
	int my_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int num_proc;
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	
	// Check if the number of processes is valid
	if (YSIZE % num_proc != 0){
		if (my_rank == 0) {
			printf("ERROR: the number of processes is not a multiple of %d\n", YSIZE); 
		}
		
		// Finalize MPI
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
	
		return 0;
	}
	
	// Root proceess reads the image
	int image_size = XSIZE * YSIZE * 3;
	uchar *image = NULL;
	if (my_rank == 0) {
		uchar *image = calloc(image_size, 1); // Three uchars per pixel (RGB)
		readbmp("before.bmp", image);
		printf("Process %d read the image succesfully\n", my_rank);
	}
	
	// Allocate a subset of the image for each of the processes
	int image_chunk_size = (YSIZE / num_proc) * XSIZE * 3;
	uchar *subset = calloc(image_chunk_size, 1);
	printf("Process %d allocated chunk size of %d succesfully\n", my_rank, image_chunk_size);

	// Scatter image between the processes. It will produce an array called subset with a piece of the image in each process
	/*MPI_Scatter(image, 				// Data on root process
				image_chunk_size, 	// Number of elements sent to each process
				MPI_UNSIGNED_CHAR, 	// Data type
				subset,				// Where to place the scattered data
				image_chunk_size, 	// Number of elements to receive
				MPI_UNSIGNED_CHAR,  // Data type
				0, 					// Rank of root process
				MPI_COMM_WORLD);	// MPI communicator*/
	printf("Process %d scattered succesfully\n", my_rank);
	
	// Do something to each data chunck
	
	// Gather the results from the processes. It will take all the image pieces and place them in output_image
	uchar *output_image = NULL;
	if (my_rank == 0) {
		output_image = calloc(image_size, 1);
	}
	MPI_Gather(&subset, 			// Data to gather
				image_chunk_size, 	// Number of elements of each gathered data
				MPI_UNSIGNED_CHAR, 	// Data type
				output_image, 		// Where to place gathered data
				image_chunk_size, 	// Number of elements recevied per process
				MPI_UNSIGNED_CHAR,  // Data type
				0, 					// Rank of root process
				MPI_COMM_WORLD);	// MPI communicator
	
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
