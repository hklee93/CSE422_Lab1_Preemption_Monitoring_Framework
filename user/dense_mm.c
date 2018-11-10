/******************************************************************************
*
* dense_mm.c
*
* This program implements a dense matrix multiply and can be used as a
* hypothetical workload.
*
* Usage: This program takes a single input describing the size of the matrices
*        to multiply. For an input of size N, it computes A*B = C where each
*        of A, B, and C are matrices of size N*N. Matrices A and B are filled
*        with random values.
*
* Written Sept 6, 2015 by David Ferry
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>


const int num_expected_args = 2;
const unsigned sqrt_of_UINT32_MAX = 65536;

int main( int argc, char* argv[] ){

	unsigned index, row, col; //loop indicies
	unsigned matrix_size, squared_size;
	double *A, *B, *C;
	int fd, status, copied;
	unsigned long long prev_on_core, prev_btw, btw, on_core;

	int cur_cpu, migration, counter;

	preemption_info_t *data = malloc(sizeof(preemption_info_t));

	if( argc != num_expected_args ){
		printf("Usage: ./dense_mm <size of matrices>\n");
		exit(-1);
	}

	matrix_size = atoi(argv[1]);

	if( matrix_size > sqrt_of_UINT32_MAX ){
		printf("ERROR: Matrix size must be between zero and 65536!\n");
		exit(-1);
	}

	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
		return -1;
	}



	squared_size = matrix_size * matrix_size;

	printf("Generating matrices...\n");

	A = (double*) malloc( sizeof(double) * squared_size );
	B = (double*) malloc( sizeof(double) * squared_size );
	C = (double*) malloc( sizeof(double) * squared_size );

	for( index = 0; index < squared_size; index++ ){
		A[index] = (double) rand();
		B[index] = (double) rand();
		C[index] = 0.0;
	}

	printf("Multiplying matrices...\n");

	status = enable_preemption_tracking(fd);
	if (status < 0) {
		fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
		close(fd);
		return -1;
	}

	for( row = 0; row < matrix_size; row++ ){
		for( col = 0; col < matrix_size; col++ ){
			for( index = 0; index < matrix_size; index++){
				C[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
			}
		}
	}

	counter = 0;
	copied = read(fd, data, sizeof(preemption_info_t));
	prev_btw = data->wait_time-1;
	prev_on_core = data->on_core_time-1;
	btw = 0;
	on_core = 0;
	counter = 1;
	cur_cpu = data->on_which_core;
	migration = 0;
	int iter = 0;

	while(data->on_core_time != prev_on_core && data->wait_time != prev_btw){
		iter++;
		copied = read(fd, data, sizeof(preemption_info_t));
		if(copied == 0){

			btw += data->wait_time;
			on_core += data->on_core_time;

			counter++;
			if(cur_cpu != data->on_which_core){

				cur_cpu = data->on_which_core;
				migration++;
			}

		}
		else if(copied == -1 && iter > 100){
			break;
		}
	}

	status = disable_preemption_tracking(fd);
	if (status < 0) {
		fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	printf("Multiplication done!\n");
	printf("Program summary:\n");
	printf("avg run time: %lluns\n", on_core / counter);
	printf("avg wait time: %lluns\n", btw / counter);
	printf("num of migrations: %d\n", migration);
	printf("total preemptions: %d\n", counter);

	return 0;
}
