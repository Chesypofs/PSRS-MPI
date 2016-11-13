#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "mpi.h"
#include "phases.h"

// taken from:
// www.cplusplus.com/reference/cstdlib/qsort/
int compareFunction(const void * a, const void * b) {
  if (*(long*)a < *(long*)b) return -1;
  if (*(long*)a > *(long*)b) return 1;
  else return 0;
}

void phase1(long *unsortedArray, long arraySize, long startIndex, long subArraySize, long *pivots, int numProcesses) {
  // sort the subarray
  qsort(unsortedArray + startIndex, subArraySize, sizeof(unsortedArray[0]), compareFunction);

  // find the pivots
  for (int i = 0; i < numProcesses; i++) {
    pivots[i] = unsortedArray[startIndex + (i * (arraySize / (numProcesses * numProcesses)))];
  }
  return;
}

void phase2(long *unsortedArray, long startIndex, long subArraySize, long *pivots, int *partitionSizes, int numProcesses, int myId) {
  long *collectedPivots = (long *) malloc(numProcesses * numProcesses * sizeof(pivots[0]));
  long *phase2Pivots = (long *) malloc((numProcesses - 1) * sizeof(pivots[0]));
  int index = 0;

  MPI_Gather(pivots, numProcesses, MPI_LONG, collectedPivots, numProcesses, MPI_LONG, 0, MPI_COMM_WORLD);
  if (myId == 0) {
    // sort the pivots
    qsort(collectedPivots, numProcesses * numProcesses, sizeof(pivots[0]), compareFunction);

    // find the new pivots
    for (int i = 0; i < (numProcesses -1); i++) {
      phase2Pivots[i] = collectedPivots[(((i+1) * numProcesses) + (numProcesses / 2)) - 1];
    }
  }
  
  MPI_Bcast(phase2Pivots, numProcesses - 1, MPI_LONG, 0, MPI_COMM_WORLD);
  // Get the partitions and their sizes
  for (long i = 0; i < subArraySize; i++) {
    if (unsortedArray[startIndex + i] > phase2Pivots[index]) {
      // starting a new partition
      index += 1;
    }
    if (index == numProcesses) {
      // we are in the last partition
      // just calculate the length till the end of the subArray
      partitionSizes[numProcesses - 1] = subArraySize - i + 1;
      break;
    }
    partitionSizes[index] += 1;
  }
  free(collectedPivots);
  free(phase2Pivots);
  return;
}

void phase3(long *unsortedArray, long startIndex, int *partitionSizes, long **newPartitions, int *newPartitionSizes, int numProcesses) {
  int totalSize = 0;
  int *sendDisp = (int *) malloc(numProcesses * sizeof(int));
  int *recvDisp = (int *) malloc(numProcesses * sizeof(int));

  // First send all the partition sizes
  MPI_Alltoall(partitionSizes, 1, MPI_INT, newPartitionSizes, 1, MPI_INT, MPI_COMM_WORLD);

  // Allocate space for the incoming partitions
  for (int i = 0; i < numProcesses; i++) {
    totalSize += newPartitionSizes[i];
  }
  *newPartitions = (long *) malloc(totalSize * sizeof(long));
  
  // Calculate buffer displacements before sending partitions
  sendDisp[0] = 0;
  recvDisp[0] = 0;
  for (int i = 1; i < numProcesses; i++) {
    sendDisp[i] = partitionSizes[i - 1] + sendDisp[i - 1];
    recvDisp[i] = newPartitionSizes[i - 1] + recvDisp[i - 1];
  }

  // Now send all the partitions
  MPI_Alltoallv(&(unsortedArray[startIndex]), partitionSizes, sendDisp, MPI_LONG, *newPartitions, newPartitionSizes, recvDisp, MPI_LONG, MPI_COMM_WORLD);

  free(sendDisp);
  free(recvDisp);
  return;
}

void phase4(long *partitions, int *partitionSizes, int numProcesses, int myId, long *unsortedArray) {
  long *sortedSubList;
  int *recvDisp, *indexes, *partitionEnds, *subListSizes, totalListSize;

  indexes = (int *) malloc(numProcesses * sizeof(int));
  partitionEnds = (int *) malloc(numProcesses * sizeof(int));
  indexes[0] = 0;
  totalListSize = partitionSizes[0];
  for (int i = 1; i < numProcesses; i++) {
    totalListSize += partitionSizes[i];
    indexes[i] = indexes[i-1] + partitionSizes[i-1];
    partitionEnds[i-1] = indexes[i];
  }
  partitionEnds[numProcesses - 1] = totalListSize;

  sortedSubList = (long *) malloc(totalListSize * sizeof(long));
  subListSizes = (int *) malloc(numProcesses * sizeof(int));
  recvDisp = (int *) malloc(numProcesses * sizeof(int));

  // Merge the partitions into one sorted sublist
  for (long i = 0; i < totalListSize; i++) {
    long lowest = LONG_MAX;
    long ind = -1;
    for (int j = 0; j < numProcesses; j++) {
      if ((indexes[j] < partitionEnds[j]) && (partitions[indexes[j]] < lowest)) {
	lowest = partitions[indexes[j]];
	ind = j;
      }
    }
    sortedSubList[i] = lowest;
    indexes[ind] += 1;
  }

  // Now send all the subList sizes back to the root
  MPI_Gather(&totalListSize, 1, MPI_INT, subListSizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // Calculate receving buffer displacement on the root
  if (myId == 0) {
    recvDisp[0] = 0;
    for (int i = 1; i < numProcesses; i++) {
      recvDisp[i] = subListSizes[i - 1] + recvDisp[i - 1];
    }
  }

  // Send the sorted subLists back to the root
  MPI_Gatherv(sortedSubList, totalListSize, MPI_LONG, unsortedArray, subListSizes, recvDisp, MPI_LONG, 0, MPI_COMM_WORLD);
  
  free(partitionEnds);
  free(sortedSubList);
  free(indexes);
  free(subListSizes);
  free(recvDisp);
  return;
}
