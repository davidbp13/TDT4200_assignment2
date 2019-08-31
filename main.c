#include <stdlib.h>
#include <stdio.h>
#include <mpi.h> // To use MPI APIs
#include "bitmap.h"

#define XSIZE 2560 // Size of before image
#define YSIZE 2048

/*Idea:
 * Take a scatter/gather example as a base
 * Process 0 will verify if the number of process is valid
 * Process 0 will liad the image
 * Process 0 will use scatter to split the rows among the number of processes
 * Each process will perform some kind of computation in their own row and return the finished row
 * Process 0 will gather all the results and save the image
*/

int main(int argc, char** argv) {
	MPI_Init(NULL, NULL); // MPI Initialization
	
	// Get the id of each process and the total amount of processes
	int my_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	int num_proc;
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	
	// Root proceess reads the image
	uchar *image = NULL;
	if (my_rank == 0) {
		uchar *image = calloc(XSIZE * YSIZE * 3, 1); // Three uchars per pixel (RGB)
		readbmp("before.bmp", image);
	}
	
	// Allocate a subset of the image for each of the processes
	uchar *subset = (uchar *)malloc(sizeof(uchar) * XSIZE * 3);
	//assert(subset != NULL);

	// Scatter image between the processes
	MPI_Scatter(image,
				XSIZE*3, 
				MPI_UNSIGNED_CHAR, 
				subset,
				XSIZE*3, 
				MPI_UNSIGNED_CHAR, 
				0, 
				MPI_COMM_WORLD);
	
	// Gather the results from the processes
	uchar *output_image = NULL;
	if (my_rank == 0) {
		output_image = (uchar *)malloc(sizeof(uchar) * XSIZE * YSIZE * 3);
		//assert(output_image != NULL);
	}
	MPI_Gather(&subset, 
				XSIZE*3, 
				MPI_UNSIGNED_CHAR, 
				output_image, 
				XSIZE*3, 
				MPI_UNSIGNED_CHAR, 
				0, 
				MPI_COMM_WORLD);
	
	/*// Master process
	if (my_rank == 0){
		// Process 0 will verify that the number of processes is right, otherwise it will show and error and finish
		if (YSIZE % num_proc != 0) {
			printf("ERROR: the number of processes is not a multiple of %d\n", YSIZE); 
		}
		else{
			
		}
	else{
		//Other processes
		printf("Hey we still are doing stuff :)\n"); 
	}
	*/
	
	// Root proceess saves the image
	if (my_rank == 0) {
		savebmp("after.bmp", image, XSIZE, YSIZE);
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
