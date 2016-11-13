#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include "mpi.h"
#include "phases.h"

int main(int argc, char *argv[]) {
  int numProcesses, myId, *partitionSizes, *newPartitionSizes, nameLength;
  long subArraySize, arraySize, *unsortedArray, startIndex, endIndex, *pivots, *newPartitions;
  int numArrays = 6;
  int numRuns = 5;
  char processorName[MPI_MAX_PROCESSOR_NAME];
  double times[numRuns][5], avgTimes[5];

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numProcesses);
  MPI_Comm_rank(MPI_COMM_WORLD,&myId);
  MPI_Get_processor_name(processorName,&nameLength);

  fprintf(stdout,"Process %d of %d is on %s\n",
	  myId, numProcesses, processorName);
  fflush(stdout);
  
  arraySize = strtol(argv[1], NULL, 10);
  unsortedArray = (long *) malloc(arraySize*sizeof(long));
  for (int i = 0; i < 5; i++) {
    avgTimes[i] = 0;
  }

  for (int i = 0; i < numRuns; i++) {
    srandom(99);
    for (long k = 0; k < arraySize; k++) {
      unsortedArray[k] = random();
    }

    pivots = (long *) malloc(numProcesses*sizeof(long));
    partitionSizes = (int *) malloc(numProcesses*sizeof(int));
    newPartitionSizes = (int *) malloc(numProcesses*sizeof(int));
    for (int k = 0; k < numProcesses; k++) {
      partitionSizes[k] = 0;
    }

    // Get start and size of subarray
    startIndex = myId * arraySize / numProcesses;
    if (numProcesses == (myId + 1)) {
      endIndex = arraySize;
    } 
    else {
      endIndex = (myId + 1) * arraySize / numProcesses;
    }
    subArraySize = endIndex - startIndex;

    MPI_Barrier(MPI_COMM_WORLD);
    times[i][0] = MPI_Wtime();
    phase1(unsortedArray, arraySize, startIndex, subArraySize, pivots, numProcesses);
    times[i][1] = MPI_Wtime();
    if (numProcesses > 1) {
      phase2(unsortedArray, startIndex, subArraySize, pivots, partitionSizes, numProcesses, myId);
      times[i][2] = MPI_Wtime();
      phase3(unsortedArray, startIndex, partitionSizes, &newPartitions, newPartitionSizes, numProcesses);
      times[i][3] = MPI_Wtime();
      phase4(newPartitions, newPartitionSizes, numProcesses, myId, unsortedArray);
      times[i][4] = MPI_Wtime();
    }
      
    if (myId == 0) {
      for (long k = 0; k < arraySize - 1; k++) {
	assert(unsortedArray[k] <= unsortedArray[k + 1]);
      }
      if (numProcesses > 1) {
	avgTimes[0] += times[i][1] - times[i][0];
	avgTimes[1] += times[i][2] - times[i][1];
	avgTimes[2] += times[i][3] - times[i][2];
	avgTimes[3] += times[i][4] - times[i][3];
	avgTimes[4] += times[i][4] - times[i][0];
      } else {
	avgTimes[4] += times[i][1] - times[i][0];
      }
    }
    if (numProcesses > 1) {
      free(newPartitions);
    }
    free(partitionSizes);
    free(newPartitionSizes);
    free(pivots);
  }
  for (int i = 0; i < 5; i++) {
    avgTimes[i] = avgTimes[i] / 5;
  }
  if (myId == 0) {
    if (numProcesses > 1) {
      printf("NUM PROCESSES: %i   TOTAL TIME: %fs   P1 TIME: %fs   P2 TIME: %fs   P3 TIME: %fs   P4 TIME: %fs\n", numProcesses, avgTimes[4], avgTimes[0], avgTimes[1], avgTimes[2], avgTimes[3]);
    }
    else {
      printf("NUM PROCESSES: %i   TOTAL TIME: %fs\n", numProcesses, avgTimes[4]);
    }
  }
  free(unsortedArray);
  MPI_Finalize();
  return 0;
}
