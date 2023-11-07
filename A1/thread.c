/*
Family Name: Vaidya
Given Name: Daivakshi
Student Number: 217761016
CS Login: vaidya28
YorkU email address (the one that appears in eClass): vaidya28@my.yorku.ca
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_LIMIT 1000

//initialise some variables that are shared by all threads (parent and child)
float sum;
int x;

/* This is the "runner" function, it is a worker thread that gets executed
by each thread, the param is the filename of the dataset that needs to be 
worked upon and it is passed by the main thread. */
void * workerThreadFunction(void * param){
    char * fileName = (char *)param;    //typecast into string
    FILE * file;
    //allocate space in memory for the float numbers on file
    float * numbers = malloc(sizeof(float) * MAX_LIMIT);
    sum = 0;
    x = 0;
    // this part of code reads the file and stores all floats into an array
    file = fopen(fileName, "r");
    //loop stores into array while also counting the number of floats in the file
    while (fscanf(file, "%f", &numbers[x]) != EOF)
    {
        x++;
    }
    fclose(file);
    //indicates end of array when whole file is finished reading
    numbers[x] = '\0';
    int j;
    //calculates the sum of all floats stored in the array
    for (j = 0; numbers[j] != '\0'; j++){
        sum += numbers[j];
    }
    pthread_exit(0);    //exit child thread
}

int main(int argc, char **argv){

    int i;

    float sumTot[argc];
    int num[argc];
    float avg[argc];

    pthread_t tid[(argc - 1)];  //an array of thread id's
    pthread_attr_t attr;        //set of attributes for worker thread
    pthread_attr_init(&attr);   //to initialize attributes to default

    for (i = 0;  i < argc-1; i++){

        //create worker threads and pass the filename as parameter
        pthread_create(&tid[i], &attr, workerThreadFunction, argv[i+1]);
        //main thread waits until all child threads are finished executing
        pthread_join(tid[i], NULL);

        sumTot[i] = sum;    //array containing sums from all threads
        num[i] = x;         //array containing number of float from all threads

    }

    float finalAvg = 0.0;   //final average from all worker threads

    /* loop calculates averages from sum and num for each thread and stores
    results in an array */
    for (i = 0; i < argc-1; i++){
        avg[i] = sumTot[i]/((float)num[i]);
        printf("Filename %s SUM=%f NUM=%d AVG=%f\n", argv[i+1], sumTot[i], num[i], avg[i]);
        finalAvg += avg[i];
    }

    finalAvg = finalAvg/(argc - 1);
    printf("AVERAGE=%f\n", finalAvg);
     

    pthread_exit(0);    //exit main thread
    return 0;

}