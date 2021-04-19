/*
*   Author: Kyle Brown
*   Date: 4/19/2021
*   CS 470 Operating Systems Lab 2
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

void printResults(int * results) {

    int totalChars = 0;
    for (int i = 0; i < 27; i++) {
        totalChars += results[i];
    }

    float aZChars = ((totalChars - results[26]) / (float)totalChars) * 100;
    float nonAZChars = (results[26] / (float)totalChars) * 100;
    printf("[ -------------------------------------------------- ]\n");
    printf("Total number of characters is: %d\n", totalChars);
    printf("The number of Aa-Zz characters is: %d\t frequency: (%.2f %%)\n", totalChars - results[26], aZChars);
    printf("The number of non Aa-Zz characters: %d\t frequency: (%.2f %%)\n", results[26], nonAZChars);
    printf("------------------------------------------------------\n");

    // printing values of all Aa-Zz characters
    for (int i = 0; i < 27; i++) {
        if (i != 26) {
            char upperCase = (char) i + 65;
            char lowerCase = (char) i + 97;
            float frequency = (results[i] / (float)(totalChars - results[26])) * 100;
            printf("The number of (%c's or %c's) is: %d\t frequency: (%.2f %%)\n", lowerCase, upperCase, results[i], frequency);  
        }  
    }
}

int *checkFrequencies(char *fname) {

    pid_t pids[27];
    int numChildProcs = 26;
    int *shared_memory = mmap(NULL, sizeof(int) * 27, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for (int i = 0; i < numChildProcs; i++) {
        
        pids[i] = fork();

        FILE *file = fopen(fname, "r");
        if (file == NULL) {
            printf("An error occured trying to open the file with pid: %d\n", getpid());
            exit(1);
        }

        // Aborting the child process if an error occurs
        if (pids[i] < 0) {
            printf("Error with process: %d\n", getpid());
            abort();
        }

        // child processes
        else if (pids[i] == 0) {

            char c;
            while ((c = fgetc(file)) != EOF) {
                // Non-ASCII character encountered (rarer)
                if (c < 0 || c > 127) {
                    continue;
                }

                else if (c == (char) i + 65 || c == (char) i + 97) {
                    shared_memory[i]++;
                }
            }
            fclose(file);
            exit(0);
        }
    }

    //  The main process waits for all children processes to finish
    int status;
    pid_t pid;
    while (numChildProcs > 0) {
        pid = wait(&status);
        --numChildProcs;
    }

    // The main process counts all non Aa-Zz characters before exiting 
    FILE *file = fopen(fname, "r");
    if (file == NULL) {
        printf("An error occured trying to open the file with pid: %d\n", getpid());
        exit(1);
    }

    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c < 0 || c > 127) {
            shared_memory[26]++;
        }
        else if (!islower(c) || !isupper(c)) {
            shared_memory[26]++;
        }
    }

    fclose(file);
    return shared_memory;
}

int main(int argc, char* argv[]) {
    
    int exitCode = 0;
    
    if (argc < 2) {
        printf("Insufficient number of arguments!\nA file name must be provided after the program name.\n");
    }

    else if (argc > 2) {
        printf("Too many arguments given\nUsage: <program name> <filename.extension>\n\n");
    }

    else {
        printf("Filename given: %s\n", argv[1]);

        FILE *file;
        file = fopen(argv[1], "r");

        if (file != NULL) {
            int *numChars = checkFrequencies(argv[1]);

            printResults(numChars);

            // Frees mmap shared memory that was allocated
            int error = munmap(numChars, sizeof(int) * 27);
            if (error != 0) {
                printf("An error occured with unmapping shared memory\n");
                exitCode = 1;
            }

            fclose(file);
        }
        else {
            exitCode = 1;
            printf("File not found at the specified path!\nprogram exited with code %d\n", exitCode);
        }
    }

    return exitCode;
}