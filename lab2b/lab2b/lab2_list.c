// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

// Globals to make life easier for helper functions. mutexLock placed here
// to share among all threads.
int numThreads = 1;
int numIterations = 1;
int opt_yield = 0;
int numLists = 1;
int numElements;
char syncOption;

SortedList_t *list;
SortedListElement_t *elements;
pthread_mutex_t *mutexLocks;
long long *threadLockTimes;
long *locks;


// Prototypes
char* generateKey(void);
void* handleThreadStuff(void* args);
static inline unsigned long getNanoseconds(struct timespec * spec);
void handleSegfault(int val);
void recordTime(struct timespec* clock);



int main(int argc, char * argv[]) {
    // Establish defaults & set up clock
    struct timespec start, stop;
    char tagName[20] = "list-";
    
    // Catch any segfaults that may occur while the threads are running
    // Put up here because for some reason, this didn't register completely for all threads
    // when placed lower
    signal(SIGSEGV, handleSegfault);
    
    // Option parsing (based off setup from lab0)
    struct option args[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {"lists", required_argument, NULL, 'l'},
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
                // If yield option was used, scan through the entire argument
                // Change opt_yield as needed
                opt_yield = 1;
                for (int k = 0; k < (int)strlen(optarg); k++) {
                    if (optarg[k] == 'i')
                        opt_yield = opt_yield | INSERT_YIELD;
                    else if (optarg[k] == 'd')
                        opt_yield = opt_yield | DELETE_YIELD;
                    else if (optarg[k] == 'l')
                        opt_yield = opt_yield | LOOKUP_YIELD;
                    else {
                        fprintf(stderr, "Invalid yield option. Valid options are i, d, or l\n");
                        exit(1);
                    }
                }
                break;
            
            case 's': // --sync
                if ((*optarg == 's') || (*optarg == 'm')) {
                    syncOption = *optarg;
                } else {
                    fprintf(stderr, "Invalid sync option. Valid options are s or m\n");
                    exit(1);
                }
                break;
            
            case 'l': // --lists
                numLists = atoi(optarg);
                
                if (numLists <= 0) {
                    fprintf(stderr, "Invalid list value. Must be positive. \n");
                    exit(1);
                }
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab2a_list [--threads=#] [--iterations=#] [--yield=OPTIONS] [--sync=OPTION]\n");
                exit(1);
        }
    }

    
    // Set tagName to the proper name based on synchronization and yield options
    // Handling yield naming first
    //char tagName[20] = "list-";
    if (opt_yield) {
        if (opt_yield & INSERT_YIELD)
            strcat(tagName, "i");
        if (opt_yield & DELETE_YIELD)
            strcat(tagName, "d");
        if (opt_yield & LOOKUP_YIELD)
            strcat(tagName, "l");
    } else {
        strcat(tagName, "none");
    }
    
    // Handling sync naming second
    if (syncOption == 'm')
        strcat(tagName, "-m");
    else if (syncOption == 's')
        strcat(tagName, "-s");
    else
        strcat(tagName, "-none");
    
    // Begin initializing the lists based off numLists
    list = (SortedList_t *) malloc(numLists * sizeof(SortedList_t));
    mutexLocks = malloc(numLists * sizeof(pthread_mutex_t));
    locks = malloc(numLists * sizeof(long));
    
    // Prepare list heads
    for (int k = 0; k < numLists; k++) {
        list[k].next = &list[k];
        list[k].prev = &list[k];
        list[k].key = NULL;
    }
    
    // Prepare either mutex or spin-lock (if applicable)
    if (syncOption == 'm') {
        for (int k = 0; k < numLists; k++)
            pthread_mutex_init(&mutexLocks[k], NULL);
    }
    if (syncOption == 's') {
        for (int k = 0; k < numLists; k++) {
            locks[k] = 0;
        }
    }
    
    // Prepare elements and initialize with random keys
    numElements = numThreads * numIterations;
    elements = malloc(sizeof(SortedListElement_t) * numElements);
    for (int k = 0; k < numElements; k++) {
        elements[k].key = generateKey();
    }
    
    // Prep threads and get start time
    pthread_t threads[numThreads];
    int threadID[numThreads];
    threadLockTimes = malloc(numThreads * sizeof(long long));
    recordTime(&start);
    
    // Create all threads
    for (int k = 0; k < numThreads; k++) {
        threadID[k] = k;
        if (pthread_create(&threads[k], NULL, handleThreadStuff, &threadID[k]) == 1) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(errno));
            exit(2);
        }
    }
    
    // Wait for all threads to exit
    for (int k = 0; k < numThreads; k++) {
        if (pthread_join(threads[k], NULL) == 1) {
            fprintf(stderr, "Error waiting on thread: %s\n", strerror(errno));
            exit(2);
        }
    }
    
    // Get stop time
    recordTime(&stop);
    
    // Prepares everything else for the CSV
    int totalOperations = numElements * 3;
    long totalRunTime = getNanoseconds(&stop) - getNanoseconds(&start);
    long avgPerOperation = totalRunTime/totalOperations;
    
    // Prepare time
    long long totalLockTime = 0;
    long long avgLockTime = 0;
    for (int k = 0; k < numThreads; k++) {
        totalLockTime += threadLockTimes[k];
    }
    avgLockTime = totalLockTime/totalOperations;
    
    // Output CSV
    fprintf(stdout, "%s,%d,%d,%d,%d,%ld,%ld,%lld\n", tagName, numThreads, numIterations, numLists, totalOperations, totalRunTime, avgPerOperation, avgLockTime);
    
    exit(0);
}

// Generate a random key
char* generateKey(void) {
    // Generate a random size that isn't larger than 10
    int size = rand() % 10;
    char *alphanumeric = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    char *key = (char*) malloc(size + 1);
    for (int k = 0; k < size; k++) {
        key[k] = alphanumeric[rand() % strlen(alphanumeric)];
    }
    
    // cstrings end with \0
    key[size] = '\0';
    return key;
}

// Provided by TA Slides, gets the nanoseconds from a timespec
static inline unsigned long getNanoseconds(struct timespec * spec) {
    unsigned long ret= spec->tv_sec; //seconds
    ret = ret * 1000000000 + spec->tv_nsec; //nanoseconds
    return ret;
}

void* handleThreadStuff(void* args) {
    // Prepare variables and resources that may needed for locks.
    int threadID = *(int*)args;
    struct timespec threadStart, threadStop;
    int length = 0;
    
    // Handle insertion of all the element based on syncOption
    for (int k = threadID; k < numElements; k += numThreads) {
        int hash = elements[k].key[0] % numLists;
        
        // If mutex option selected
        if (syncOption == 'm') {
            recordTime(&threadStart);
            pthread_mutex_lock(&mutexLocks[hash]);
            recordTime(&threadStop);
            
            SortedList_insert(&list[hash], &elements[k]);
            pthread_mutex_unlock(&mutexLocks[hash]);
        }
        
        // If spinlock option selected
        else if (syncOption == 's') {
            recordTime(&threadStart);
            while(__sync_lock_test_and_set(&locks[hash], 1));
            recordTime(&threadStop);
            
            SortedList_insert(&list[hash], &elements[k]);
            __sync_lock_release(&locks[hash]);
        }
        
        // No sync option specified
        else {
            SortedList_insert(&list[hash], &elements[k]);
        }
        
        // Add time to threadLockTimes
        threadLockTimes[threadID] += getNanoseconds(&threadStop) - getNanoseconds(&threadStart);
    }
    
    // Handle grabbing list length based on syncOption
    // If mutex option selected
    for (int k = 0; k < numLists; k++) {
        int hash = elements[k].key[0] % numLists;
        
        if (syncOption == 'm') {
            recordTime(&threadStart);
            pthread_mutex_lock(&mutexLocks[hash]);
            recordTime(&threadStop);
            
            length += SortedList_length(&list[hash]);
            pthread_mutex_unlock(&mutexLocks[hash]);
        }
        
        // If spinlock option selected
        else if (syncOption == 's') {
            recordTime(&threadStart);
            while(__sync_lock_test_and_set(&locks[hash], 1));
            recordTime(&threadStop);
            
            length += SortedList_length(&list[hash]);
            __sync_lock_release(&locks[hash]);
        }
        
        // If no sync option specified
        else {
            length += SortedList_length(&list[hash]);
        }
        
        // Add time to threadLockTimes
           threadLockTimes[threadID] += getNanoseconds(&threadStop) - getNanoseconds(&threadStart);
    }
    
    
    
    // Handle deleting of all the element based on syncOption
    // Need to look up the element, then delete it.
    SortedListElement_t *temp;
    for (int k = threadID; k < numElements; k += numThreads) {
        int hash = elements[k].key[0] % numLists;
        
        // If mutex option selected
        if (syncOption == 'm') {
            recordTime(&threadStart);
            pthread_mutex_lock(&mutexLocks[hash]);
            recordTime(&threadStop);
            
            temp = SortedList_lookup(&list[hash], elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
            pthread_mutex_unlock(&mutexLocks[hash]);
        }
        
        // If spinlock option selected
        else if (syncOption == 's') {
            recordTime(&threadStart);
            while(__sync_lock_test_and_set(&locks[hash], 1));
            recordTime(&threadStop);
            
            temp = SortedList_lookup(&list[hash], elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
            __sync_lock_release(&locks[hash]);
        }
        
        // If no sync option specified
        else {
            temp = SortedList_lookup(&list[hash], elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
        }
        
        // Add time to threadLockTimes
        threadLockTimes[threadID] += getNanoseconds(&threadStop) - getNanoseconds(&threadStart);
    }
    
    return NULL;
}

void handleSegfault (int val) {
    if (val == SIGSEGV) {
        fprintf(stderr, "A segementation fault has occurred: %s\n", strerror(errno));
        exit(2);
    }
}

void recordTime(struct timespec* clock) {
    if (clock_gettime(CLOCK_MONOTONIC, clock) == -1) {
        fprintf(stderr, "Error getting start time: %s\n", strerror(errno));
        exit(2);
    }
}
