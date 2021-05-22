// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h>  // fprintf()
#include <stdlib.h> // exit()
#include <getopt.h> // getopt()
#include <fcntl.h>  // open(), close()
#include <errno.h>  // used for errno in error output
#include <string.h> // strerror()
#include <signal.h> // signal()
#include <unistd.h> // read(), write()


/* Exit Codes
 0 ... copy successful
 1 ... unrecognized argument
 2 ... unable to open input file
 3 ... unable to open output file
 4 ... caught and received SIGSEGV */

void handleSegFault(int sig) {
    // If statement needed as error was given when compiling (sig wasn't used)
    if (sig == SIGSEGV) {
        fprintf(stderr, "A segmentation error was caught.\n");
        exit(4);
    }
}

int main(int argc, char * argv[]) {
    // Argument Flags. Input/Output set equal to NULL while segFault and catch flags
    // are set equal to 0. Changes will be made via parsing later if necessary
    char* inputFile = NULL;
    char* outputFile = NULL;
    int segfaultFlag = 0;
    int catchFlag = 0;
    
    // A struct that holds information used for option parsing.
    // no_argument = 0, required_argument = 1, optional_argument = 2
    struct option args[] = {
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"segfault", no_argument, NULL, 's'},
        {"catch", no_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };
    
    
    // val is used for the return values of getopt
    int val;
    
    // Goes through the command line options and sets the appropriate flags
    // getopt() only returns -1 when it's finished
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 'i': // --input
                inputFile = optarg;
                break;
                
            case 'o': // --output
                outputFile = optarg;
                break;
                
            case 's': // --segfault
                segfaultFlag = 1;
                break;
            
            case 'c': // --catch
                catchFlag = 1;
                break;
            
            default: // Bad argument
                fprintf(stderr, "Usage: lab0 [--input=file] [--output=file] [--segfault] [--catch]\n");
                exit(1);
        }
    }
    
    // If input option was used
    if (inputFile != NULL) {
        // Opens inputFile with only reading permissions
        int ifd = open(inputFile, O_RDONLY);
        
        // If >= 0, success --> do I/O redirection so that 0 points to ifd
        // instead of stdin. Else (< 0), there was an error
        if (ifd >= 0) {
            close(0);
            dup(ifd);
            close(ifd);
        } else {
            fprintf(stderr, "Error opening input file %s: %s\n", inputFile, strerror(errno));
            exit(2);
        }
    }
    
    // If output option was used
    if (outputFile != NULL) {
        // Opens outputFile. The three or'd values are grouped together in the man page
        // for open() when creating something.
        // O_CREAT: If outputFile doesn't exist, create it with 0666 permissions
        // O_WRONGLY: Write only permissions
        // O_TRUNC: If file is regular, it exists and you have write permissions, truncate to length 0
        int ifd = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC , 0666);
        
        // If >= 0, success --> do I/O redirection so that 1 points to ifd
        // instead of stdin. Else (< 0), there was an error
        if (ifd >= 0) {
            close(1);
            dup(ifd);
            close(ifd);
        } else {
            fprintf(stderr, "Error opening output file %s: %s\n", outputFile, strerror(errno));
            exit(3);
        }
    }
    
    // If catch option was used
    if (catchFlag != 0) {
        // In the event of a segmentation fault, handleSegFault will be run now
        // instead of the default handler
        signal(SIGSEGV, handleSegFault);
    }
    
    // If segfault option was used, cause a segmentation fault
    if (segfaultFlag != 0) {
        char* destructo = NULL;
        *destructo = 10;
    }
    
    // If we reached this point, no segmentation fault occurred. Read from 0 (this will
    // either be stdin if --input wasn't used OR a file if it was used) and write to
    // 1 (will either be stdout if --ouput wasn't used OR a file if it was used).
    // Code below reads/writes individual characters until EOF
    char c;
    ssize_t bytesRead = read(0, &c, sizeof(char));
    while (bytesRead > 0) {
        write(1, &c, sizeof(char));
        bytesRead = read(0, &c, sizeof(char));
    }
    
    return 0;
}
