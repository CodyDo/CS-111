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

// Globals for ease of use with helper functions. mutexLock placed here
// to share among all threads
long long counter = 0;
int numIterations = 1;
int opt_yield = 0;
char syncOption = 'x';
pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;
long lock = 0;

// Prototypes
void add(long long *pointer, long long value);
void* handleThreadStuff(void* args);
static inline unsigned long getNanoseconds(struct timespec * spec);

int main(int argc, char * argv[]) {
    // Establish defaults & set up clock
    int numThreads = 1;
    struct timespec start, stop;
    char tagName[20] = "add-";
    
    // Option parsing (based off setup from lab0)
    struct option args[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", no_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };
    
    // val used to hold return value of getopt_long
    int val;
    
    // Parse through the options, acting as necessary.
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 't': // --threads
                numThreads = atoi(optarg);
                break;
            
            case 'i': // --iterations
                numIterations = atoi(optarg);
                break;
                
            case 'y': // --yield
                opt_yield = 1;
                break;
            
            case 's': // --sync
                if ((*optarg == 's') || (*optarg == 'm') || (*optarg == 'c')) {
                    syncOption = *optarg;
                } else {
                    fprintf(stderr, "Invalid sync option. Valid options are s, m, or c\n");
                    exit(1);
                }
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab2a_add [--threads=#] [--iterations=#] [--yield] [--sync=OPTION]\n");
                exit(1);
        }
    }
    
    // Set tagName to the proper name based on synchronization and yield options.
    /*
     m = mutex
     s = spin lock
     c = compare-and-swap
     */
    if (opt_yield) {
        strcat(tagName, "yield-");
        if (syncOption == 'x')
            strcat(tagName, "none");
        else if (syncOption == 'm')
            strcat(tagName, "m");
        else if (syncOption == 's')
            strcat(tagName, "s");
        else if (syncOption == 'c')
            strcat(tagName, "c");
    } else {
        if (syncOption == 'x')
            strcat(tagName, "none");
        else if (syncOption == 'm')
            strcat(tagName, "m");
        else if (syncOption == 's')
            strcat(tagName, "s");
        else  if (syncOption == 'c')
            strcat(tagName, "c");
        
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
    int totalOperations = numThreads * numIterations * 2;
    long totalRunTime = getNanoseconds(&stop) - getNanoseconds(&start);
    long avgPerOperation = totalRunTime/totalOperations;
    
    // Output CSV
    fprintf(stdout, "%s,%d,%d,%d,%ld,%ld,%lld\n", tagName, numThreads, numIterations, totalOperations, totalRunTime, avgPerOperation, counter);
    
    exit(0);
}

// Provided by the spec
void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        if (opt_yield)
                sched_yield();
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
    // HANDLE ADDITION ASPECT
    int k;
    for (k = 0; k < numIterations; k++) {
        // Mutex option
        if (syncOption == 'm') {
            pthread_mutex_lock(&mutexLock);
            add(&counter, 1);
            pthread_mutex_unlock(&mutexLock);
        }
        
        // Spin Lock
        else if (syncOption == 's') {
            while(__sync_lock_test_and_set(&lock, 1));
            add(&counter, 1);
            __sync_lock_release(&lock);
        }
        
        // Compare-and-swap. Based off TA's Slides
        else if (syncOption == 'c') {
            long long old, new;
            do {
                old = counter;
                new = old + 1;
                
                // Check yield prior to performing compare-and-swap
                if (opt_yield)
                    sched_yield();
            } while(__sync_val_compare_and_swap(&counter, old, new) != old);
        }
        
        // No syncOption present; normal add.
        else
            add(&counter, 1);
    }
    
    // HANDLE SUBTRACTION ASPECT. SAME AS ABOVE EXCEPT -1 IN PLACE
    // OF +1
    for (k = 0; k < numIterations; k++) {
        // Mutex option
        if (syncOption == 'm') {
            pthread_mutex_lock(&mutexLock);
            add(&counter, -1);
            pthread_mutex_unlock(&mutexLock);
        }
        
        // Spin Lock
        else if (syncOption == 's') {
            while(__sync_lock_test_and_set(&lock, 1));
            add(&counter, -1);
            __sync_lock_release(&lock);
        }
        
        // Compare-and-swap. Based off TA's Slides
        else if (syncOption == 'c') {
            long long old, new;
            do {
                old = counter;
                new = old - 1;
                
                // Check yield prior to performing compare-and-swap
                if (opt_yield)
                    sched_yield();
            } while(__sync_val_compare_and_swap(&counter, old, new) != old);
        }
        
        // No syncOption present; normal add.
        else
            add(&counter, -1);
    }
    
    return args;
}
