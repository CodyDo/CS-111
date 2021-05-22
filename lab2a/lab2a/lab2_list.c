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
long lock = 0;
int numElements;
char syncOption;
SortedList_t *list;
SortedListElement_t *elements;
pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;



// Prototypes
char* generateKey(void);
void* handleThreadStuff(void* args);
static inline unsigned long getNanoseconds(struct timespec * spec);
void handleSegfault(int val);


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
    
    // Begin initializing the list.
    list = malloc(sizeof(SortedList_t));
    list->next = list;
    list->prev = list;
    list->key = NULL;
    
    numElements = numThreads * numIterations;
    elements = malloc(sizeof(SortedListElement_t) * numElements);
    
    for (int k = 0; k < numElements; k++) {
        elements[k].key = generateKey();
    }
    
    // Prep threads and get start time
    pthread_t threads[numThreads];
    int threadID[numThreads];
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        fprintf(stderr, "Error getting start time: %s\n", strerror(errno));
        exit(2);
    }
    
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
       if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
        fprintf(stderr, "Error getting start time: %s\n", strerror(errno));
        exit(2);
    }
    
    // Prepares everything else for the CSV
    int totalOperations = numElements * 3;
    long totalRunTime = getNanoseconds(&stop) - getNanoseconds(&start);
    long avgPerOperation = totalRunTime/totalOperations;
    
    // Check to make sure that the linked list is size 0
    if (SortedList_length(list) != 0) {
        fprintf(stderr, "Length of the linked list is not zero\n");
        exit(2);
    }
    
    // Output CSV
    fprintf(stdout, "%s,%d,%d,%d,%d,%ld,%ld\n", tagName, numThreads, numIterations, 1, totalOperations, totalRunTime, avgPerOperation);
    
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
    
    // Handle insertion of all the element based on syncOption
    for (int k = threadID; k < numElements; k += numThreads) {
        // If mutex option selected
        if (syncOption == 'm') {
            pthread_mutex_lock(&mutexLock);
            SortedList_insert(list, &elements[k]);
            pthread_mutex_unlock(&mutexLock);
        }
        
        // If spinlock option selected
        else if (syncOption == 's') {
            while(__sync_lock_test_and_set(&lock, 1));
            SortedList_insert(list, &elements[k]);
            __sync_lock_release(&lock);
        }
        
        // No sync option specified
        else
            SortedList_insert(list, &elements[k]);
    }
    
    // Handle grabbing list length based on syncOption
    // If mutex option selected
    if (syncOption == 'm') {
        pthread_mutex_lock(&mutexLock);
        SortedList_length(list);
        pthread_mutex_unlock(&mutexLock);
    }
    
    // If spinlock option selected
    else if (syncOption == 's') {
        while(__sync_lock_test_and_set(&lock, 1));
        SortedList_length(list);
        __sync_lock_release(&lock);
    }
    
    // If no sync option specified
    else {
        SortedList_length(list);
    }
    
    
    // Handle deleting of all the element based on syncOption
    // Need to look up the element, then delete it.
    SortedListElement_t *temp;
    for (int k = threadID; k < numElements; k += numThreads) {
        // If mutex option selected
        if (syncOption == 'm') {
            pthread_mutex_lock(&mutexLock);
            temp = SortedList_lookup(list, elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
            pthread_mutex_unlock(&mutexLock);
        }
        
        // If spinlock option selected
        else if (syncOption == 's') {
            while(__sync_lock_test_and_set(&lock, 1));
            temp = SortedList_lookup(list, elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
            __sync_lock_release(&lock);
        }
        
        // If no sync option specified
        else {
            temp = SortedList_lookup(list, elements[k].key);
            
            // Check if valid
            if (temp == NULL) {
                fprintf(stderr, "Error using SortedList_lookup to delete element in list");
                exit(2);
            }
            
            // Delete if valid then unlock
            SortedList_delete(temp);
        }
    }
    
    return NULL;
}

void handleSegfault (int val) {
    if (val == SIGSEGV) {
        fprintf(stderr, "A segementation fault has occurred: %s\n", strerror(errno));
        exit(2);
    }
}
