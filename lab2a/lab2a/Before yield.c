// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// Globals for ease of use with helper functions
long long counter = 0;
int numIterations = 1;

// Prototypes
void add(long long *pointer, long long value);
void* handleThreadStuff(void* args);
static inline unsigned long getNanoseconds(struct timespec * spec);

int main(int argc, char * argv[]) {
    // Establish defaults & set up clock
    int numThreads = 1;
    struct timespec start, stop;
    
    // Option parsing (based off setup from lab0)
    struct option args[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", no_argument, NULL, 'y'},
        {0, 0, 0, 0}
    };
    
    // val used to hold return value of getopt_long
    int val;
    
    // Parse through the options and see if "--shell" is present. If another option
    // exists, exit with error
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 't': // --threads
                numThreads = atoi(optarg);
                break;
            
            case 'i': // --iterations
                numIterations = atoi(optarg);
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab2a_add [--threads=#] [--iterations=#]\n");
                exit(1);
        }
    }
    
    // Prep threads and get start time
    pthread_t threads[numThreads];
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        fprintf(stderr, "Error getting start time: %s\n", strerror(errno));
        exit(2);
    }
    
    // Create all threads
    int k;
    for (k = 0; k < numThreads; k++) {
        if (pthread_create(&threads[k], NULL, handleThreadStuff, NULL) == 1) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(errno));
            exit(2);
        }
    }
    
    // Wait for all threads to exit
    for (k = 0; k < numThreads; k++) {
        if (pthread_join(threads[k], NULL) == 1) {
            fprintf(stderr, "Error waiting on thread: %s\n", strerror(errno));
            exit(2);
        }
    }
    
    // Get stop time
       if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
        fprintf(stderr, "Error getting start time: %s\n", strerror(errno));
        exit(2);
    }
    
    // Prepares everything for the CSV
    char* testName = "add-none";
    int totalOperations = numThreads * numIterations * 2;
    long totalRunTime = getNanoseconds(&stop) - getNanoseconds(&start);
    long avgPerOperation = totalRunTime/totalOperations;
    
    // Output CSV
    fprintf(stdout, "%s,%d,%d,%d,%ld,%ld,%lld\n", testName, numThreads, numIterations, totalOperations, totalRunTime, avgPerOperation, counter);
    
    exit(0);
}

// Provided by the spec
void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        *pointer = sum;
}

// Provided by TA Slides, gets the nanoseconds from a timespec
static inline unsigned long getNanoseconds(struct timespec * spec) {
    unsigned long ret= spec->tv_sec; //seconds
    ret = ret * 1000000000 + spec->tv_nsec; //nanoseconds
    return ret;
}

// The function that each thread will call.
void* handleThreadStuff(void* args) {
    int k;
    for (k = 0; k < numIterations; k++) {
        add(&counter, 1);
        add(&counter, -1);
    }
    
    return NULL;
}
