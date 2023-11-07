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
int numberOfProcesses = 0;              // total number of processes 
int nextProcess = 0;                    // index of the next process
int totalWaitingTime = 0;               // total waiting time
int totalContextSwitches = 0;           // total number of context switches
int cpuTimeUtilized = 0;                // amout of CPU time utilised
int clock = 0;                          // total time passed
int sumTurnarounds = 0;                 // sum of all turnaround values
int timeQuantum;

// Ready process queue and waiting process queue
process_queue readyQueue;
process_queue waitingQueue;

// CPU's
process *CPUS[NUMBER_OF_PROCESSORS];

// Temporary "Pre-Ready" queue
process *tmpQueue[MAX_PROCESSES+1];
int tmpQueueSize = 0;


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

//Calculates average CPU utilization in terms of percentages
double averageUtilizationTime(int utilizationTime){
 	double result = (utilizationTime * 100.0) / clock;
 	return result;
}

//Return the total number of incoming processes that have yet to arrive in the system
int totalIncomingProcesses(void){
	return numberOfProcesses - nextProcess;
}

//Compare arrival time diffference of two processes
int compareArrivalTime(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	return first->arrivalTime - second->arrivalTime;
}

//Compare process IDs of two processes
int compareProcessIds(const void *a, const void *b){
	process *first = (process *) a;
	process *second = (process *) b;
	if (first->pid == second->pid){
		error_duplicate_pid(first->pid);
	}
	return first->pid - second->pid;
}

/**
 * Iterates over all CPU's and to find and return the total number of 
 * currently running processes
 */
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

/**
 * Grabs the next scheduled process in the queue (first process currently at
 * the front of the ready queue). Increments the waiting time in order to update
 * the ready state. Returns the next process to be run
 */
process *nextScheduledProcess(void){
	if (readyQueue.size == 0){
		return NULL;
	}
	process *grabNext = readyQueue.front->data;
	dequeueProcess(&readyQueue);
	return grabNext;
}

/**
 * Add any new incoming processes to a temporary queue to be sorted and later added
 * to the ready queue. These incoming processes are put in a "pre-ready queue"
 */
void addNewIncomingProcess(void){
	while(nextProcess < numberOfProcesses && processes[nextProcess].arrivalTime <= clock){
		tmpQueue[tmpQueueSize] = &processes[nextProcess];
		tmpQueue[tmpQueueSize]->quantumRemaining = timeQuantum;
		tmpQueueSize++;
		nextProcess++;
	}
}

/**
 * Get the first process in the waiting queue, check if their I/O burst is complete.
 * If the current I/O burst is complete, move on to next I/O burst and add the process
 * to the "pre-ready queue". Dequeue the waiting queue and update waiting state by 
 * incrementing the current burst's step
 */
void waitingToReady(void){
 	int i;
 	int waitingQueueSize = waitingQueue.size;
 	for(i = 0; i < waitingQueueSize; i++){
 		process *grabNext = waitingQueue.front->data;
 		dequeueProcess(&waitingQueue);
 		if(grabNext->bursts[grabNext->currentBurst].step == grabNext->bursts[grabNext->currentBurst].length){
 			grabNext->currentBurst++;
 			grabNext->quantumRemaining = timeQuantum;
 			grabNext->endTime = clock;
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
 		enqueueProcess(&readyQueue, tmpQueue[i]);
 	}
	tmpQueueSize = 0;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (CPUS[i] == NULL){
 			CPUS[i] = nextScheduledProcess();
 		}
 	}
 }

/**
 * If a currently running process has finished their CPU burst, move them to the waiting queue 
 * and terminate those who have finished their CPU burst. Start the process' next I/O burst. If 
 * CPU burst is not finished, move the process to the waiting queue and free the current CPU. 
 * If the CPU burst is finished, terminate the process by setting the end time to the current 
 * simulation time. Alternatively, the time slice expires before the current burst is over.
 */
void runningToWaiting(void){
	int num = 0;
	process *preemptive[NUMBER_OF_PROCESSORS];
 	int i, j;
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if (CPUS[i] != NULL){
 			if (CPUS[i]->bursts[CPUS[i]->currentBurst].step == CPUS[i]->bursts[CPUS[i]->currentBurst].length){
 				CPUS[i]->currentBurst++;
 				if (CPUS[i]->currentBurst < CPUS[i]->numberOfBursts){
 					enqueueProcess(&waitingQueue, CPUS[i]);
 				}
 				else{
 					CPUS[i]->endTime = clock;
 				}
 				CPUS[i] = NULL;
 			}
 			// context switch takes longer than time slice
 			else if(CPUS[i]->quantumRemaining == 0){
 				preemptive[num] = CPUS[i];
 				preemptive[num]->quantumRemaining = timeQuantum;
 				num++;
 				totalContextSwitches++;
 				CPUS[i] = NULL;
 			}
 		}	
 	}
 	// sort preemptive processes by process ID's and enqueue in the ready queue
 	qsort(preemptive, num, sizeof(process*), compareProcessIds);
 	for (j = 0; j < num; j++){
 		enqueueProcess(&readyQueue, preemptive[j]);
 	}
 }

 //Update waiting processes, ready processes, and running processes
 void updateStates(void){
 	int i;
 	int waitingQueueSize = waitingQueue.size;
 	// update waiting state
 	for (i = 0; i < waitingQueueSize; i++){
 		process *grabNext = waitingQueue.front->data;
 		dequeueProcess(&waitingQueue);
 		grabNext->bursts[grabNext->currentBurst].step++;
 		enqueueProcess(&waitingQueue, grabNext);
 	}
 	// update ready process
 	for (i = 0; i < readyQueue.size; i++){
 		process *grabNext = readyQueue.front->data;
 		dequeueProcess(&readyQueue);
 		grabNext->waitingTime++;
 		enqueueProcess(&readyQueue, grabNext);
 	}
 	// update CPU's
 	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
 		if(CPUS[i] != NULL){
 			CPUS[i]->bursts[CPUS[i]->currentBurst].step++;
 			CPUS[i]->quantumRemaining--;
 		}
 	}
 }

// print results
void displayResults(float awt, float atat, int sim, float aut, int cs, int pids){
	printf("\n------------------ Round Robin (rr) ------------------\n"
		"Average waiting time\t\t:%.2f units\n"
		"Average turnaround time\t\t:%.2f units\n"
		"Time CPU finished all processes\t:%d\n"
		"Average CPU utilization\t\t:%.1f%%\n"
		"Number of context Switces\t:%d\n" 
		"PID of last process to finish\t:%d\n"
		"------------------------------------------------------\n\n", awt, atat, sim, aut, cs, pids);
}

int main(int argc, char *argv[]){
	int i;
	int status = 0;
	double ut, wt, tat;
	int lastPID;
	timeQuantum = atoi(argv[1]);

	// input error handling
	if (argc > 2){
		printf("Incorrect number of arguments, only add one time slice.\n");
		exit(-1);
	}
	else if (argc < 2){
		printf("Must add time slice.\n");
		exit(-1);
	}

	// clear CPU'S, initialize queues, and reset global variables
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		CPUS[i] = NULL;
	}
	initializeProcessQueue(&readyQueue);
	initializeProcessQueue(&waitingQueue);

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
		runningToWaiting();
		readyToRunning();
		waitingToReady();
		
		updateStates();

		// break when there are no more running or incoming processes, and the waiting queue is empty
		if (runningProcesses() == 0 && totalIncomingProcesses() == 0 && waitingQueue.size == 0){
			break;
		}

		cpuTimeUtilized += runningProcesses();
		clock++;
	}

	// calculations
	for(i = 0; i < numberOfProcesses; i++){
		sumTurnarounds +=processes[i].endTime - processes[i].arrivalTime;
		totalWaitingTime += processes[i].waitingTime;

		if (processes[i].endTime == clock){
			lastPID = processes[i].pid;
		}
	}

	wt = averageWaitTime(totalWaitingTime);
	tat = averageTurnaroundTime(sumTurnarounds);
	ut = averageUtilizationTime(cpuTimeUtilized);

	displayResults(wt, tat, clock, ut, totalContextSwitches, lastPID);
	
	return 0;
}