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
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define TEN_MILLIS_IN_NANOS 10000000

int bufSize = 0;
FILE *file_in;
FILE *file_out;
FILE *file_log;

// mutex locks for threads
pthread_mutex_t mutex;
pthread_mutex_t mutexREAD;
pthread_mutex_t mutexPRODUCE;
pthread_mutex_t mutexCONSUME;
pthread_mutex_t mutexWRITE;
pthread_mutex_t mutexLOG;
sem_t empty;
sem_t full;


typedef  struct {
     char  data;
     off_t offset;
     char state;
} BufferItem;

BufferItem *circularBuffer;

//Puts a thread to sleep (whether IN or OUT) for a time between 0 to 0.01 seconds
void thread_sleep(void){
	struct timespec t;
	t.tv_sec = 0;
    t.tv_nsec = rand()%(TEN_MILLIS_IN_NANOS+1);
	nanosleep(&t, NULL);
}

// error handling for valid user input
void valid_input(int param, int expected, char* msg){
	if (param < expected){
		fprintf(stderr, "%s\n", msg);
		exit(-1);
	}
}

// check if the buffer is empty
int is_buffer_empty(){
	int i = 0;
	while (i < bufSize){
		if (circularBuffer[i].state == 'e'){
			return 1;
		}
		i++;
	}
	return 0;
}

// return the first empty item in the buffer
int first_empty_item_in_buffer(){
	int i = 0;
	if (is_buffer_empty()){
		while (i < bufSize){
			if (circularBuffer[i].state == 'e'){
				return i;
			}
			i++;
		}
	}
	return -1;
}

// return the first item to be written to the file from the buffer
int first_out_item_in_buffer(){
	int i = 0;
	while (i < bufSize){
		if (circularBuffer[i].state == 'o'){
			return i;
		}
		i++;
	}
	return -1;
}

// initialize the buffer - set all states to empty
void initialize_buffer(){
	int i = 0;
	while (i < bufSize){
		circularBuffer[i].state = 'e';
		i++;
	}
}


// the in thread - read from file and write to buffer
void *IN_thread(void * param){

    int threadID = *((int *) param);
	int index = first_empty_item_in_buffer();
	char curr;
	off_t offset;

	thread_sleep();


	while (1){

		// if the buffer is empty - sleep
		if (is_buffer_empty() == 1) {
			thread_sleep();
		}

        //READ

		// critical section to read in file and store character
		pthread_mutex_lock(&mutexREAD);

        if ((offset = ftell(file_in)) < 0) {
            pthread_mutex_unlock(&mutexREAD);
    		fprintf(stderr, "IN: Cannot seek output file \n");
			exit(1);
		}
		if((fread(&curr, sizeof(curr), 1, file_in)) < 1) {
			printf("read_byte PT%d EOF pthread_exit(0)\n", threadID);
            pthread_mutex_unlock(&mutexREAD);
   			pthread_exit(0);
		}

        //Write operation details into the log file
        pthread_mutex_lock(&mutexLOG);
        fprintf(file_log,"read_byte PT%d O%d B%d I-1\n", threadID, (unsigned int)offset, (unsigned int)curr);
        pthread_mutex_unlock(&mutexLOG);

        pthread_mutex_unlock(&mutexREAD);
        


        //PRODUCE

        sem_wait(&empty);
		pthread_mutex_lock(&mutexPRODUCE);
        
		//store character to buffer and indicate that it is ready for work then grab next item
        circularBuffer[index].offset = offset;
		circularBuffer[index].data = curr;
		circularBuffer[index].state = 'o';

        //Write operation details into the log file
        pthread_mutex_lock(&mutexLOG);
        fprintf(file_log,"produce PT%d O%d B%d I%d\n", threadID, (unsigned int)offset, (unsigned int)curr, index);
		pthread_mutex_unlock(&mutexLOG);

        //index = (index + 1)%bufSize;
        index = first_empty_item_in_buffer();

        pthread_mutex_unlock(&mutexPRODUCE);
        sem_post(&full);

	}

	thread_sleep();
	return NULL;
}


// the output thread - read from buffer and write to file
void *OUT_thread(void * param){
    int threadID = *((int *) param);
	int index = first_out_item_in_buffer();
	char curr;
	off_t offset;

	thread_sleep();

	while (1){

        // if the buffer is empty - sleep
		if (is_buffer_empty() == 1) {
			thread_sleep();
		}

        //CONSUME

		// store that current character - read from buffer
        sem_wait(&full);
        pthread_mutex_lock(&mutexCONSUME);

		offset = circularBuffer[index].offset;
		curr = circularBuffer[index].data;
        
        // write operation details to log file
        pthread_mutex_lock(&mutexLOG);
        fprintf(file_log,"consume CT%d O%d B%d I%d\n", threadID, (unsigned int)offset, (unsigned int)curr, index);
        pthread_mutex_unlock(&mutexLOG);

        pthread_mutex_unlock(&mutexCONSUME);
        sem_post(&empty);
		

        //WRITE

        // critical section for writing to file
		pthread_mutex_lock(&mutexWRITE);
		if (fseek(file_out, offset, SEEK_SET) < 0) {
            pthread_mutex_unlock(&mutexWRITE);
    		fprintf(stderr, "OUT%d: Cannot seek output file\n", threadID);
			exit(1);
		}
		if (fputc(curr, file_out) == EOF) {	
            pthread_mutex_unlock(&mutexWRITE);
			fprintf(stderr, "Cannot write to output file\n");
   			exit(1);
		}

        //write operation details to log file
        pthread_mutex_lock(&mutexLOG);
        fprintf(file_log,"write_byte CT%d O%d B%d I-1\n", threadID, (unsigned int)offset, (unsigned int)curr);
        pthread_mutex_unlock(&mutexLOG);

        pthread_mutex_unlock(&mutexWRITE);

		// store empty character to buffer and indicate that it is empty
	    circularBuffer[index].data = '\0';
	    circularBuffer[index].state = 'e';
		circularBuffer[index].offset = 0;
        
		index = first_out_item_in_buffer();
        
	}

	thread_sleep();
	return NULL;
}

int main(int argc, char *argv[]){
	int i, j;
	int nIN;
	int nOUT;
	
	// initialize all mutexes
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexREAD, NULL);
	pthread_mutex_init(&mutexPRODUCE, NULL);
	pthread_mutex_init(&mutexCONSUME, NULL);
	pthread_mutex_init(&mutexWRITE, NULL);
    pthread_mutex_init(&mutexLOG, NULL);

	// read in arguments
	file_in = fopen(argv[3], "r");
	file_out = fopen(argv[4], "w");
    file_log = fopen(argv[6], "w");
	nIN = atoi(argv[1]);
	nOUT = atoi(argv[2]);
	bufSize = atoi(argv[5]);

	// threads
	pthread_t INthreads[nIN];
	*INthreads = (pthread_t)malloc(sizeof(pthread_t)*nIN);
	pthread_t OUTthreads[nOUT];
	*OUTthreads = (pthread_t)malloc(sizeof(pthread_t)*nOUT);
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// create a buffer item and initialize the buffer
	circularBuffer = (BufferItem*)malloc(sizeof(BufferItem)*bufSize);
	initialize_buffer();

    sem_init(&empty, 0, bufSize);
    sem_init(&full, 0, 0);

	// error handling for user input in the CLI
	valid_input(nIN, 1, "number of IN threads should be at least 1");
	valid_input(nOUT, 1, "number of OUT threads should be at least 1");
	valid_input(bufSize, 1, "buffer size should be at least 1");
	valid_input(argc, 7, "follow this format: ./cpy <nIN> <nOUT> <file> <copy> <bufSize> <Log>");

	if (file_in == NULL){
		fprintf(stderr, "could not open input file for reading\n");
	}
	if (file_out == NULL){
		fprintf(stderr, "could not open input file for writing\n");
	}
    if (file_log == NULL){
		fprintf(stderr, "could not open input file for writing\n");
	}

    int *arg = malloc(sizeof(*arg));
    if ( arg == NULL ) {
        fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
        exit(EXIT_FAILURE);
    }

	// create as many in/out threads as specified
    printf("\nCreating Threads ... \n");
	for (i = 0; i < nIN; i++){
        //printf("In create %d\n", i);
        *arg = i;
        if (pthread_create(&INthreads[i], &attr, (void *) IN_thread, arg) != 0) {
            perror("Failed to create thread");
        }
	}
	for (j = 0; j < nOUT; j++){
        //printf("Out create %d\n", j);
        *arg = j;
        if (pthread_create(&OUTthreads[j], &attr, (void *) OUT_thread, arg) != 0) {
            perror("Failed to create thread");
        }
	}

	// join all the threads
    printf("Joining Threads ... \n");
	for (i = 0; i < nIN; i++){
        if (pthread_join(INthreads[i], NULL) != 0) {
            perror("Failed to join IN thread");
        }
	}

	// destory all mutexes
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutexREAD);
	pthread_mutex_destroy(&mutexPRODUCE);
	pthread_mutex_destroy(&mutexCONSUME);
	pthread_mutex_destroy(&mutexWRITE);
    pthread_mutex_destroy(&mutexLOG);
    sem_destroy(&empty);
    sem_destroy(&full);

	// close all files and free buffer
	fclose(file_in);
	fclose(file_out);
    fclose(file_log);
	free(circularBuffer);

    printf("Process Complete \n\n");

	return 0;
}