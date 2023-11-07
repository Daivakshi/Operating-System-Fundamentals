# EECS-3221---Assignment-1

/* Assignment Requirements:

You are required to write several C programs to process some text documents. In natural language processing, it is an interesting idea to represent each word by a real-numbered vector in a high-dimension space. Your programs will first compute the pair-wise similarities for all pairs of two different words in a given subset, and then search to find the top 3 most similar word pairs among all possible pairs in the subset. Finally, the program will print the results according to a particular format as specified below. Each text document consists of some English words followed by the corresponding word vectors (one word per line). The file format is that all words and numbers are separated by whitespace (space, tab or newline).  As example, we have posted three data sets on the course Web: wordvector1.txt, wordvector2.txt, wordvector3.txt, for your reference. However, your program should be able to process any number of text documents that are passed through the command line.

Part 1: rvw.c

As the first step of your assignment, you should write a regular Ansi C program (just like those ones you did in EECS2031), called rvw.c, to correctly print out the top 3 most similar word pairs for all data files provided in the command line. If you have more than one data file, your program should process them sequentially and output results one after another. The output of each data file includes filename, the most similar pair of words and their cosine similarity, the 2nd most similar pair and the cosine similarity, and the 3rd most similar pair and the cosine similarity.  And the outputs for one data file occupy exactly one line as follows:
Filename1 wordA11 wordB11 simA11B11 wordA12 wordB12 simA12B12 wordA13 wordB13 simA13B13
Filename2 wordA21 wordB21 simA21B21 wordA22 wordB22 simA22B22 wordA23 wordB23 simA23B23
…
 
They are printed out in the same order in the command line.

Part 2: process.c 

Write your second C program, called process.c, to print the most similar word pairs concurrently using multiple processes. The program first reads file names of all data sets from the command line. For each data set, it creates a child process. Each child process first reads all data from the file, and searches for the top 3 most similar word pairs in that data set, and sends the found results to the main process via a Unix pipe. It is required that all child processes need to process the assigned datasets concurrently, not one by one sequentially. The main program will print out all these results along with the file name of the data set to the standard output. If you have more than one data set, the outputs should be printed out in the same order in the command line. The output of one data set occupies one line as follows:
Filename1 wordA11 wordB11 simA11B11 wordA12 wordB12 simA12B12 wordA13 wordB13 simA13B13
Filename2 wordA21 wordB21 simA21B21 wordA22 wordB22 simA22B22 wordA23 wordB23 simA23B23

Part 3: thread.c

Write another Pthread C program, called thread.c, to repeat the above task concurrently using multiple threads. The program creates one pthread to process each data set. Each thread searches for the top 3 most similar word pairs and for each data set and passes all results to the main program. It is required that all child threads need to process the assigned datasets concurrently, not one by one sequentially. The main program waits for all threads to finish and print out all results following the exactly same output format as above.


*/
