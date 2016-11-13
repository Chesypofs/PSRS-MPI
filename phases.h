#pragma once

int compareFunction(const void * a, const void * b);
void phase1(long *unsortedArray, long arraySize, long startIndex, long subArraySize, long *pivots, int numProcesses);
void phase2(long *unsortedArray, long startIndex, long subArraySize, long *pivots, int *partitionSizes, int numProcesses, int myId);
void phase3(long *unsortedArray, long startIndex, int *partitionSizes, long **newPartitions, int *newPartitionSizes, int numProcesses);
void phase4(long *partitions, int *partitionSizes, int numProcesses, int myId, long *unsortedArray);
