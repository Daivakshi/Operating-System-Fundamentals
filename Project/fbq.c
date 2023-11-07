/*
Family Name: Vaidya
Given Name: Daivakshi
Student Number: 217761016
CS Login: vaidya28
YorkU email address (the one that appears in eClass): vaidya28@my.yorku.ca
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "sch-helpers.h"


process processes[MAX_PROCESSES+1];
int numberOfProcesses;              	// total number of processes
int nextProcess;                    	// index of the next process
int totalWaitingTime;               	// total waiting time
int totalContextSwitches;           	// total number of context switches
int cpuTimeUtilized;                	// amout of CPU time utilised
int theClock;                       	// total time passed
int sumTurnarounds;                 	// sum of all turnaround values
int currentLevel;
int timeQuantums[NUMBER_OF_LEVELS-1];

// Ready process queues and waiting process queues
process_queue readyQueue[NUMBER_OF_LEVELS];
process_queue waitingQueue;

// CPU's
process *CPUS[NUMBER_OF_PROCESSORS];

// Temporary "Pre-Ready" queue
process *tmpQueue[MAX_PROCESSES+1];
int tmpQueueSize;

//Resets all global variables to 0
void resetVariables(void){
	numberOfProcesses = 0;
	nextProcess = 0;
	totalWaitingTime = 0;
	totalContextSwitches = 0;
	cpuTimeUtilized = 0;
	theClock = 0;
	sumTurnarounds = 0;
	tmpQueueSize = 0;
	currentLevel= 0;
}

//Calulates average wait time
double averageWaitTime(int waitTime){
	double result = waitTime / (double) numberOfProcesses;
	return result;
}

//Calculates average turnaround time
double averageTurnaroundTime(int turnaroundTime){
	double result = turnaroundTime / (double) numberOfProcesses;
	return result;
}

//Calculates average CPU utilization
double averageUtilizationTime(int utilizationTime){
 	double result = (utilizationTime * 100.0) / theClock;
 	return result;
}

//Return the total number of incoming processes that have yet to arrive in the system
int totalIncomingProcesses(void){
	return numberOfProcesses - nextProcess;
}

//Compare arrival time of two processes
int compareArrivalTime(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	return first->arrivalTime - second->arrivalTime;
}

//Compare process ID of two processes
int compareProcessIds(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	if (first->pid == second->pid){
		error_duplicate_pid(first->pid);
	}
	return first->pid - second->pid;
}

//Compare priorities of two processes
int comparePriority(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	if (first->priority == second->priority){
		return compareProcessIds(first, second);
	}
	return first->priority - second->priority;
}

//Iterates over all CPU's and to find and return the total number of currently running processes
int runningProcesses(void){
	int runningProcesses = 0;
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if (CPUS[i] != NULL){
			runningProcesses++;
		}
	}
	return runningProcesses;
}

//Grabs the next process to be run based on the availability in their ready queues.
process *nextScheduledProcess(void){
	int readyQueueSize1 = readyQueue[0].size;
	int readyQueueSize2 = readyQueue[1].size;
	int readyQueueSize3 = readyQueue[2].size;
	process *grabNext = NULL;
	
	if (readyQueueSize1 != 0){
		grabNext = readyQueue[0].front->data;
		dequeueProcess(&readyQueue[0]);
	}
	else if (readyQueueSize2 != 0){
		grabNext = readyQueue[1].front->data;
		dequeueProcess(&readyQueue[1]);
	} 
	else if (readyQueueSize3 != 0){
		grabNext = readyQueue[2].front->data;
		dequeueProcess(&readyQueue[2]);
	}
	return grabNext; 
}

/**
 * Add any new incoming processes to a temporary queue to be sorted and later added
 * to the ready queue. These incoming processes are put in a "pre-ready queue"
 */
void addNewIncomingProcess(void){
	while(nextProcess < numberOfProcesses && processes[nextProcess].arrivalTime <= theClock){
		tmpQueue[tmpQueueSize] = &processes[nextProcess];
		tmpQueueSize++;
		nextProcess++;
	}
}

/**
 * Get the first process in the waiting queue, check if their I/O burst is complete.
 * If the current I/O burst is complete, move on to next I/O burst and add the process
 * to the "pre-ready queue". Dequeue the waiting queue and update waiting state by 
 * incrementing the current burst's step. Assign priority to 0, and time quantum to 0.
 */
void waitingToReady(void){
 	int i;
 	int waitingQueueSize = waitingQueue.size;
 	for(i = 0; i < waitingQueueSize; i++){
 		process *grabNext = waitingQueue.front->data;
 		grabNext->priority = 0;
 		grabNext->quantumRemaining = 0;
 		dequeueProcess(&waitingQueue);
 		if(grabNext->bursts[grabNext->currentBurst].step == grabNext->bursts[grabNext->currentBurst].length){
 			grabNext->currentBurst++;
 			grabNext->endTime = theClock;
 			tmpQueue[tmpQueueSize++] = grabNext;
 		}
 		else{
 			enqueueProcess(&waitingQueue, grabNext);
 		}
 	}
}

/**
 * Sort elements in "pre-ready queue" in order to add them to the ready queue
 * in the proper order. Enqueue all processes in "pre-ready queue" to ready queue.
 * Reset "pre-ready queue" size to 0. Find a CPU that doesn't have a process currently 
 * running on it and schedule the next process on that CPU
 */
void readyToRunning(void){
 	int i;
 	qsort(tmpQueue, tmpQueueSize, sizeof(process*), compareProcessIds);
 	for (i = 0; i < tmpQueueSize; i++){
 		enqueueProcess(&readyQueue[0], tmpQueue[i]);
 	}
	tmpQueueSize = 0;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (CPUS[i] == NULL){
 			CPUS[i] = nextScheduledProcess();
 		}
 	}
}

/**
 * Check all cases; first that the first time slice hasn't expired, if it has then move
 * process to the second level. Run the process on that level, if the time slice expires
 * then move it to the fcfs part of the algorithm. There it will operate as a regular fcfs
 * algorithm, processing each process in a first come first serve manner.
 */
void runningToWaiting(void){
	int readyQueueSize1 = readyQueue[0].size;
	int readyQueueSize2 = readyQueue[1].size;
	int readyQueueSize3 = readyQueue[2].size;
 	int i;
 	
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (CPUS[i] != NULL){
 			if (CPUS[i]->bursts[CPUS[i]->currentBurst].step != CPUS[i]->bursts[CPUS[i]->currentBurst].length 
 				&& CPUS[i]->quantumRemaining != timeQuantums[0] && CPUS[i]->priority == 0){
 				CPUS[i]->quantumRemaining++;
 				CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 			}
 			else if(CPUS[i]->bursts[CPUS[i]->currentBurst].step != CPUS[i]->bursts[CPUS[i]->currentBurst].length 
 				&& CPUS[i]->quantumRemaining == timeQuantums[0] && CPUS[i]->priority == 0){
 				CPUS[i]->quantumRemaining = 0;
 				CPUS[i]->priority = 1;
 				totalContextSwitches++;
 				enqueueProcess(&readyQueue[1], CPUS[i]);
 				CPUS[i] = NULL;
 			}
 			else if(CPUS[i]->bursts[CPUS[i]->currentBurst].step != CPUS[i]->bursts[CPUS[i]->currentBurst].length 
 				&& CPUS[i]->quantumRemaining != timeQuantums[1] && CPUS[i]->priority == 1){
 				if (readyQueueSize1 != 0){
 					CPUS[i]->quantumRemaining = 0;
 					totalContextSwitches++;
 					enqueueProcess(&readyQueue[1], CPUS[i]);
 					CPUS[i] = NULL;
 				}
 				else{
 					CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 					CPUS[i]->quantumRemaining++;
 				}
 			}
 			else if(CPUS[i]->bursts[CPUS[i]->currentBurst].step != CPUS[i]->bursts[CPUS[i]->currentBurst].length 
 				&& CPUS[i]->quantumRemaining == timeQuantums[1] && CPUS[i]->priority == 1){
 				CPUS[i]->quantumRemaining = 0;
 				CPUS[i]->priority = 2;
 				totalContextSwitches++;
 				enqueueProcess(&readyQueue[2], CPUS[i]);
 				CPUS[i] = NULL;
 			}
 			else if(CPUS[i]->bursts[CPUS[i]->currentBurst].step != CPUS[i]->bursts[CPUS[i]->currentBurst].length 
 				&& CPUS[i]->priority == 2){
 				if (readyQueueSize1 != 0 || readyQueueSize2 != 0){
 					CPUS[i]->quantumRemaining = 0;
 					totalContextSwitches++;
 					enqueueProcess(&readyQueue[2], CPUS[i]);
 					CPUS[i] = NULL;
 				}
 				else{
 					CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 				}
 			}
 			// fcfs part of the algorithm
 			else if (CPUS[i]->bursts[CPUS[i]->currentBurst].step == CPUS[i]->bursts[CPUS[i]->currentBurst].length){
 				CPUS[i]->currentBurst++;
 				CPUS[i]->quantumRemaining = 0;
 				CPUS[i]->priority = 0;
 				if(CPUS[i]->currentBurst < CPUS[i]->numberOfBursts){
 					enqueueProcess(&waitingQueue, CPUS[i]);
 				}
 				else{
 					CPUS[i]->endTime = theClock;
 				}
 				CPUS[i] = NULL;
 			}
 		}
 		// if the CPU is free, assign it work
 		else if (CPUS[i] == NULL){
 			if(readyQueueSize1 != 0){
 				process *grabNext = readyQueue[0].front->data;
 				dequeueProcess(&readyQueue[0]);
 				CPUS[i] = grabNext;
 				CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 				CPUS[i]->quantumRemaining++;
			}
			else if(readyQueueSize2 != 0){
 				process *grabNext = readyQueue[1].front->data;
 				dequeueProcess(&readyQueue[1]);
 				CPUS[i] = grabNext;
 				CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 				CPUS[i]->quantumRemaining++;
 			}
 			else if(readyQueueSize3 != 0){
 				process *grabNext = readyQueue[2].front->data;
 				dequeueProcess(&readyQueue[2]);
 				CPUS[i] = grabNext;
 				CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 				CPUS[i]->quantumRemaining = 0;
 			}
 		}	
 	}
}

// Update waiting processes, ready processes, and running processes
 void updateStates(void){
 	int i,j;
 	int waitingQueueSize = waitingQueue.size;
 	// update waiting state
 	for (i = 0; i < waitingQueueSize; i++){
 		process *grabNext = waitingQueue.front->data;
 		dequeueProcess(&waitingQueue);
 		grabNext->bursts[grabNext->currentBurst].step++;
 		enqueueProcess(&waitingQueue, grabNext);
 	}
 	// update ready processes
 	for (i = 0; i < NUMBER_OF_LEVELS; i++){
 		for (j = 0; j < readyQueue[i].size; j++){
 			process *grabNext = readyQueue[i].front->data;
 			dequeueProcess(&readyQueue[i]);
 			grabNext->waitingTime++;
 			enqueueProcess(&readyQueue[i], grabNext);
 		}
 	}
}

/**
 * Display results for average waiting time, average turnaround time, the time
 * the CPU finished all processes, average CPU utilization, number of context
 * switches, and the process ID of the last process to finish.
 */
void displayResults(float awt, float atat, int sim, float aut, int cs, int pids){
	printf("\n-------------- Feedback Queue (fbq) ---------------\n"
		"Average waiting time\t\t:%.2f units\n"
		"Average turnaround time\t\t:%.2f units\n"
		"Time CPU finished all processes\t:%d\n"
		"Average CPU utilization\t\t:%.1f%%\n"
		"Number of context Switces\t:%d\n" 
		"PID of last process to finish\t:%d\n"
		"---------------------------------------------------\n\n", awt, atat, sim, aut, cs, pids);
}

int main(int argc, char *argv[]){
	int i;
	int status = 0;
	double ut, wt, tat;
	int lastPID;
	timeQuantums[0] = atoi(argv[1]);
	timeQuantums[1] = atoi(argv[2]);

	// input error handling
	if (argc > 3){
		printf("Incorrect number of arguments, only add two time slices.\n");
		exit(-1);
	}
	else if (argc < 3){
		printf("Must add two time slices.\n");
		exit(-1);
	}

	// clear CPU'S, initialize queues, and reset global variables
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		CPUS[i] = NULL;
	}
	for (i = 0; i < NUMBER_OF_LEVELS; ++i){
		initializeProcessQueue(&readyQueue[i]);
	}
	initializeProcessQueue(&waitingQueue);
	resetVariables();

	// read in workload and store processes
	while( (status = (readProcess(&processes[numberOfProcesses]))) ){
		if (status == 1){
			numberOfProcesses++;
		}
		if (numberOfProcesses > MAX_PROCESSES || numberOfProcesses == 0){
			error_invalid_number_of_processes(numberOfProcesses);
		}
	}
	
	qsort(processes, numberOfProcesses, sizeof(process*), compareArrivalTime);

	// main execution loop
	while (numberOfProcesses){
		addNewIncomingProcess();
		
		readyToRunning();
		runningToWaiting();
		waitingToReady();
		
		updateStates();
		// break when there are no more running or incoming processes, and the waiting queue is empty
		if (runningProcesses() == 0 && totalIncomingProcesses() == 0 && waitingQueue.size == 0){
			break;
		}

		cpuTimeUtilized += runningProcesses();
		theClock++;
	}
	// calculations
	for(i = 0; i < numberOfProcesses; i++){
		sumTurnarounds +=processes[i].endTime - processes[i].arrivalTime;
		totalWaitingTime += processes[i].waitingTime;

		if (processes[i].endTime == theClock){
			lastPID = processes[i].pid;
		}
	}

	wt = averageWaitTime(totalWaitingTime);
	tat = averageTurnaroundTime(sumTurnarounds);
	ut = averageUtilizationTime(cpuTimeUtilized);

	displayResults(wt, tat, theClock, ut, totalContextSwitches, lastPID);
	
	return 0;
}