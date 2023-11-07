/*
Family Name: Vaidya
Given Name: Daivakshi
Student Number: 217761016
CS Login: vaidya28
YorkU email address (the one that appears in eClass): vaidya28@my.yorku.ca
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LIMIT 1000

int main(int argc, char **argv)
{
    float numbers[MAX_LIMIT];   //array stores floats after reading from dataset file
    int i, j;                   //loop counters
    int counter = 0;            // (num) it counts the number of floats in each dataset
    FILE * file;
    pid_t pid;                  //process id of fork
    float sum = 0;
    int fd[2];                  //File descriptor for unix pipe

    //Throws error if pipe fails
    if(pipe(fd) == -1){
        printf("Error in pipe\n");
        return -1;
    }

    //this loop forks child processes based on argc (from command)
    for(j = 1; j < argc; ++j){
        pid = fork();       //create fork
        //if for fails it prints error message
        if (pid < 0){ 
            printf("Error in fork\n");
            return 1;
        }

        //Following is the CHILD process (what the child does)
        if (pid == 0){
            float result[2];        //this array will be passed through the pipe
            close(fd[0]);           //close pipe's read end

            //standard file reading procedure and storing floats to numbers[] array
            file = fopen(argv[j], "r");
            i = 0;
            while (fscanf(file, "%f", &numbers[i]) != EOF)
            {
                i++;
                counter++;
            }
            fclose(file);
            numbers[i] = '\0';

            //this loop calculates the sum of all floats
            for (i = 0; numbers[i] != '\0'; i++){
                sum += numbers[i];
            }
            result[0] = sum;                //first element of array stores the sum of floats
            result[1] = (float)counter;     //second element stores the number of floats in dataset
            //write result into the write end so parent can read later
            write(fd[1], &result, sizeof(float)*2);
            close(fd[1]);       //close write end of pipe once finished writing
            exit(0);            //exit the child process
        }

    }

    //PARENT process:-

    //what was written in "result" array in child process will be read through sum_num[] array
    float sum_num[2];
    float avg[argc];    //array to store averages from all datasets
    float avgTotal=0;   //to store value of total average of all averages from each dataset

    for (j = 0; j < argc-1; ++j) { 

        wait(NULL);     //waits for child process to finish running
        close(fd[1]);   //since we just want to read result from child, we close write end of pipe
        //read result from child and store into sum_num
        read(fd[0], &sum_num, sizeof(float)*2);
        avg[j] = sum_num[0]/sum_num[1];
        //print filename, sum, num and average
        printf("Filename %s SUM=%f NUM=%f AVG=%f\n", argv[j+1], sum_num[0], sum_num[1], avg[j]);

    }
    close(fd[0]);   //we no longer wand to read so we close read end as well

    //calculation for total average here
    for(j=0; j<argc-1; j++){
        avgTotal = avgTotal + avg[j];
    }
    avgTotal = avgTotal/(argc-1);

    printf("AVERAGE=%f\n", avgTotal);   //prints final average

    return 0;
}