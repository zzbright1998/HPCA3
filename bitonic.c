
#include <random>

#include <stdio.h>      // Printf
#include <time.h>       // Timer
#include <math.h>       // Logarithm
#include <stdlib.h>     // Malloc
#include "mpi.h"        // MPI Library
#include "bitonic.h"


#define MASTER 0        // Who should do the final processing?
#define OUTPUT_NUM 10   // Number of elements to display in output

// Globals
// Not ideal for them to be here though
double timer_start;
double timer_end;
int process_rank;
int num_processes;
int * array;
int array_size;

///////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////
int main(int argc, char * argv[]) {
    int i, j;

    // Initialization, get # of processes & this PID/rank
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
    //int * array;
  
     
    // Initialize Array for Storing Random Numbers
    
    array_size = ceil(atoi(argv[1]) / num_processes); //atoi string to int
    array = (int *) malloc(array_size * sizeof(int));

    std::mt19937 generator(atoi(argv[2]) + process_rank);
    /* Generate N pseudo-random uint32_t's */
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
    for (size_t i = 0; i < array_size; i++){
        array[i] = distribution(generator);
        //printf("%d ",my_array[i]);
    }

    


    // Blocks until all processes have finished generating
    MPI_Barrier(MPI_COMM_WORLD);

    // Begin Parallel Bitonic Sort Algorithm from Assignment Supplement

    // Cube Dimension
    int dimensions = (int)(log2(num_processes));

    // Start Timer before starting first sort operation (first iteration)
    if (process_rank == MASTER) {
        printf("Number of Processes spawned: %d\n", num_processes);
        timer_start = MPI_Wtime();
    }

    // Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    // Bitonic Sort follows
    for (i = 0; i < dimensions; i++) {
        for (j = i; j >= 0; j--) {
            // (window_id is even AND jth bit of process is 0)
            // OR (window_id is odd AND jth bit of process is 1)
            if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0)) {
                CompareLow(j);
            } else {
                CompareHigh(j);
            }
        }
    }

    // Blocks until all processes have finished sorting
    MPI_Barrier(MPI_COMM_WORLD);

    if (process_rank == MASTER) {
        timer_end = MPI_Wtime();
	printf("Rank: %d\n", process_rank);
        printf("Displaying sorted array (only 10 elements for quick verification)\n");
	//cout<<"size: "<<array_size<<endl;
	printf("%d ",array_size);
	printf("\n\n");
        // Print Sorting Results
        for (i = 0; i < array_size; i++) {
            if ((i % (array_size / OUTPUT_NUM)) == 0) {
                printf("%d ",array[i]);
            }
        }
        printf("\n\n");
        printf("Time Elapsed (Sec): %f\n", timer_end - timer_start);
    }


    if (process_rank == 2) {
        timer_end = MPI_Wtime();
	printf("Rank: %d\n", process_rank);
        printf("Displaying sorted array (only 10 elements for quick verification)\n");
       
	for (i = 0; i < array_size; i++) {
            if ((i % (array_size / OUTPUT_NUM)) == 0) {
                printf("%d ",array[i]);
            }
        }
   
}





    // Reset the state of the heap from Malloc
    free(array);

    // Done
    MPI_Finalize();
    return 0;
}

///////////////////////////////////////////////////
// Comparison Function
///////////////////////////////////////////////////
int ComparisonFunc(const void * a, const void * b) {
    return ( * (int *)a - * (int *)b );
}

///////////////////////////////////////////////////
// Compare Low
///////////////////////////////////////////////////
void CompareLow(int j) {
    int i, min;
printf("compare low\n");
printf("%d ",process_rank);
printf("\n");
    /* Sends the biggest of the list and receive the smallest of the list */

    // Send entire array to paired H Process
    // Exchange with a neighbor whose (d-bit binary) processor number differs only at the jth bit.
    int send_counter = 0;
    int * buffer_send =(int *) malloc((array_size + 1) * sizeof(int));
    MPI_Send(
        &array[array_size - 1],     // entire array
        1,                          // one data item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // Receive new min of sorted numbers
    int recv_counter;
    int * buffer_recieve = (int *)malloc((array_size + 1) * sizeof(int));
    MPI_Recv(
        &min,                       // buffer the message
        1,                          // one data item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );

    // Buffers all values which are greater than min send from H Process.
    for (i = 0; i < array_size; i++) {
        if (array[i] > min) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        }
    }

	printf("Low: min: %d max: %d count: %d receive: %d\n", array[0], array[array_size-1],  send_counter, min);

    buffer_send[0] = send_counter;

    // send partition to paired H process
    MPI_Send(
        buffer_send,                // Send values that are greater than min
        send_counter+1,               // # of items sent
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // receive info from paired H process
    MPI_Recv(
        buffer_recieve,             // buffer the message
        array_size+1,                 // whole array
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );
	//printf("Low Receive: %d\n", buffer_recieve[0]);
    // Take received buffer of values from H Process which are smaller than current max

    int *temp_array = (int *) malloc(array_size * sizeof(int));

    for(i=0;i<array_size;i++){
        temp_array[i]=array[i];
    }
    
    int buffer_size=buffer_recieve[0];
    int k=1;int m=0;  
   
    
    k=1;
    for (i = 0; i < array_size; i++) {

        if(temp_array[m]<=buffer_recieve[k]){
            array[i]=temp_array[m];
            m++;
        }
        else if(k<=buffer_size){
            array[i]=buffer_recieve[k];
            k++;
        }
    }

    qsort(array, array_size, sizeof(int), ComparisonFunc);

    free(buffer_send);
    free(buffer_recieve);

    return;
}


///////////////////////////////////////////////////
// Compare High
///////////////////////////////////////////////////
void CompareHigh(int j) {
    int i, max;
printf("compare high\n");
printf("%d ",process_rank);
printf("\n");
    // Receive max from L Process's entire array
    int recv_counter;
    int * buffer_recieve = (int *)malloc((array_size + 1) * sizeof(int));
    MPI_Recv(
        &max,                       // buffer max value
        1,                          // one item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );

    // Send min to L Process of current process's array
    int send_counter = 0;
    int * buffer_send = (int *)malloc((array_size + 1) * sizeof(int));
    MPI_Send(
        &array[0],                  // send min
        1,                          // one item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // Buffer a list of values which are smaller than max value
    for (i = 0; i < array_size; i++) {
        if (array[i] < max) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        } 
    }

	//printf("High: max: %d count: %d\n", max, send_counter);
	printf("High: min: %d max: %d count: %d receive: %d\n", array[0], array[array_size-1],  send_counter, max);
    // Receive blocks greater than min from paired slave
    MPI_Recv(
        buffer_recieve,             // buffer message
        array_size+1,                 // whole array
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message receiveds
    );
    recv_counter = buffer_recieve[0];

    // send partition to paired slave
    buffer_send[0] = send_counter;
    MPI_Send(
        buffer_send,                // all items smaller than max value
        send_counter+1,               // # of values smaller than max
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    int *temp_array = (int *) malloc(array_size * sizeof(int));

    for(i=0;i<array_size;i++){
        temp_array[i]=array[i];
    }

    int k=1;int m=array_size-1;
    int buffer_size=buffer_recieve[0];



    for(i=0;i<=buffer_size;i++)
    printf(" %d> ", buffer_recieve[i]);




    for (i = array_size-1; i >= 0; i--) {

        if(temp_array[m]>=buffer_recieve[k]){
            array[i]=temp_array[m];
            m--;
        }
        else if(k<=buffer_size){
            array[i]=buffer_recieve[k];
            k++;
        }
    }

    qsort(array, array_size, sizeof(int), ComparisonFunc);

    free(buffer_send);
    free(buffer_recieve);

    return;
}








































