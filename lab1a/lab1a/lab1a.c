// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h> // fprintf()
#include <stdlib.h> // exit()
#include <getopt.h> // getopt()
#include <termios.h> // All the terminal editing functions/structs
#include <errno.h>  // used for errno in error output
#include <string.h> // strerror()
#include <unistd.h> // read(), write()
#include <poll.h> // For polling functionality
#include <signal.h> // kill()
#include <sys/wait.h> //waitpid()

// Stores the original terminal modes for restoration later
struct termios originalState;

// Function prototypes. Description of what it does can be found above implementation
void prepTerminalModes(void);
void restoreTerminalModes(void);
void handleOutputNoShell(void);
void doChildStuff(void);
void doParentStuff(void);
void checkShellStatus(void);
void handleSigpipe(int val);

// These are all global variables to allow easy access for helper functions
int pipeToChild[2];
int pipeToParent[2];
pid_t pid;
int exitFlag;

// Establish uniform definitions and bunched (which is used a lot)
#define CR '\015'  // \r
#define NL '\012'  // \n
#define EOT '\004' // 0x04 End of File, not EOF because EOF already existed
#define ITR '\003' // 0x03 Interupt
char bunched[2] = {CR, NL};

int main(int argc, char * argv[]) {
    // Shell flag initially set to 0 (false). If --shell argument is present, will be
    // found later via parsing
    int shellFlag = 0;
    
    // Option parsing (based off setup from lab0)
    struct option args[] = {
        {"shell", no_argument, NULL, 's'},
        {0, 0, 0, 0}
    };
    
    // val used to hold return value of getopt_long
    int val;
    
    // Parse through the options and see if "--shell" is present. If another option
    // exists, exit with error
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 's': // --shell
                shellFlag = 1;
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab0 [--shell]\n");
                exit(1);
        }
    }
    
    // Place terminal into non-canonical input mode with no echo.
    // atexit() will cause the terminal restoration command to run prior to
    // exiting the terminal.
    prepTerminalModes();
    atexit(restoreTerminalModes);
    
    
    // Check if shell flag is true. If yes, then create pipes and react accordingly
    /*             ------> pipeToChild ----->
        [Parent]                                [Child Shell]
                   <----- pipeToParent <-----
                                                                */
    if (shellFlag) {
        // Create the pipeToChild and check for errors
        if (pipe(pipeToChild) < 0) {
            fprintf(stderr, "Error creating pipe to child shell: %s\n", strerror(errno));
            exit(1);
        }
        
        // Create the pipeToParent and check for errors
        if (pipe(pipeToParent) < 0) {
            fprintf(stderr, "Error creating pipe to parent terminal: %s\n", strerror(errno));
            exit(1);
        }
        
        // Register SIGPIPE handler for the shell
        signal(SIGPIPE, handleSigpipe);
        
        // Fork off and have the child and parent processes react appropriate
        // pid will be 0 in the child process and non-zero in the parent
        // pid will be negative if there was an error
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error forking process: %s\n", strerror(errno));
            exit(1);
            
            // Child
        } else if (pid == 0) {
            doChildStuff();
        } else {
            doParentStuff();
        }
        
        
    }
    
    // Reach here only if no --shell option.
    handleOutputNoShell();
    
    exit(0);
}


// Stores the original terminal modes in "originalState". Then places the
// terminal into non-canonical input mode with no echo
void prepTerminalModes(void) {
    
    // Gets state of fd 0 (keyboard) and stores it into originalState. If this
    // fails (returns -1) then exit with error
    if (tcgetattr(0, &originalState) < 0) {
        fprintf(stderr, "Error saving terminal modes: %s\n", strerror(errno));
        exit(1);
    }
    
    // Upon successful save, start altering the terminal modes in new struct
    struct termios newModes = originalState;
    newModes.c_iflag = ISTRIP;
    newModes.c_oflag = 0;
    newModes.c_lflag = 0;
    
    // Apply newModes to the terminal. Exit if there is an error.
    // set fd 0 (input) to newModes. TCSANOW = Changes occur immediately
    if (tcsetattr(0, TCSANOW, &newModes) < 0) {
        fprintf(stderr, "Error setting new terminal modes: %s\n", strerror(errno));
        exit(1);
    }
}

// Restores the terminal modes that were originally saved in prepTerminalModes
void restoreTerminalModes(void) {
    if (tcsetattr(0, TCSANOW, &originalState) < 0) {
        fprintf(stderr, "Error restoring original terminal modes: %s\n", strerror(errno));
        exit(1);
    }
    
}

// Read from file descriptor 0 and write to file descriptor 1
void handleOutputNoShell(void) {
    // Read from input, write to output
    char buffer[256];
    ssize_t bytesRead = read(STDIN_FILENO, buffer, 256);
    
    // Error checking bytesRead. If < 0, there's an error.
    if (bytesRead < 0 ) {
        fprintf(stderr, "Error reading terminal input: %s\n", strerror(errno));
        exit(1);
    }
    
    while (bytesRead > 0) {
        // Loop through everything inside the buffer and check what it is
        // React appropriately if it's \r, \n, EOF, and ^D. Otherwise, output
        int k;
        for (k = 0; k < bytesRead; k++) {
            // currChar holds the current character being looked at
            char currChar = buffer[k];
            
            // React to '\r' and '\n' in the same manner (write \r\n)
            if (currChar == CR || currChar == NL) {
                ssize_t checkMe = write(STDOUT_FILENO, &bunched, 2);
                if (checkMe < 0) {
                    fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                    exit(1);
                }
            }
            
            // React to EOF (0x04)
            else if (currChar == EOT) {
                ssize_t checkMe = write(STDOUT_FILENO, &bunched, 2);
                if (checkMe < 0) {
                    fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                    exit(1);
                }
                exit(0);
            }
            
            // Otherwise, just print
            else {
                ssize_t checkMe = write(STDOUT_FILENO, &currChar, 1);
                if (checkMe < 0) {
                    fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                    exit(1);
                }
            }
        }
        
        bytesRead = read(STDIN_FILENO, buffer, 256);
        if (bytesRead < 0 ) {
            fprintf(stderr, "Error reading terminal input: %s\n", strerror(errno));
            exit(1);
        }
    }
}

// Handles all the code that the child process will run
void doChildStuff(void) {
    // Diagram shown on discussion 2 slides help with the visualization
    // Close unnecessary pipes (left side on the diagram)
    close(pipeToChild[1]);
    close(pipeToParent[0]);
    
    // Redirect keyboard input to the shell (shell reads keyboard)
    close(STDIN_FILENO);
    dup(pipeToChild[0]);
    close(pipeToChild[0]);
    
    // Redirect output/stderr of the terminal to that of the shell
    close(STDOUT_FILENO);
    dup(pipeToParent[1]);
    close(STDERR_FILENO);
    dup(pipeToParent[1]);
    close(pipeToParent[1]);
    
    // Now execute the shell with no arguments
    execlp("/bin/bash", "/bin/bash", NULL);
    
    // Everything from this point onward should NOT run assuming execlp
    // was properly executed. If it does reach here, exit with error.
    fprintf(stderr, "Error executive shell in child process: %s\n", strerror(errno));
    exit(1);
}

// Handles all the code that the parent process will run
void doParentStuff(void) {
    // Close unnecessary pipes (right side of the diagram in week 2 disc slides)
    close(pipeToChild[0]);
    close(pipeToParent[1]);
    
    
    // Prepare polling functionality. Referred to discussion week slides
    // for proper syntax.
    struct pollfd fds[2] = {
        {STDIN_FILENO, POLLIN, 0},
        {pipeToParent[0], POLLIN, 0}
    };
    
    exitFlag = 0;

    while (!exitFlag) {
        // ret holds the number of file descriptors that match the events (POLLIN)
        int ret = poll(fds, 2, 0);
        if (ret > 0) {

            /* check that stdin has pending input */
            if (fds[0].revents == POLLIN) {
                // Create buffer and read into it. Check for errors
                char buffer[256];
                ssize_t bytesRead = read(STDIN_FILENO, &buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from parent pipe:%s\n", strerror(errno));
                    exit(1);
                }
                
                // Loop through everything that is read
                int k;
                for (k = 0; k < bytesRead; k++) {
                    char currChar = buffer[k];
                    if (currChar == EOT) {
                        exitFlag = 1;

                        } else if (currChar == ITR) {
                            if (kill(pid, SIGINT) < 0) {
                                fprintf(stderr, "An error with sending a SIGINT: %s\n", strerror(errno));
                                exit(1);
                            }

                        } else if (currChar == CR || currChar == NL) {
                            char solo = NL;
                            ssize_t checkMe = write(STDOUT_FILENO, &bunched, 2);
                            ssize_t checkMe2 = write(pipeToChild[1], &solo, 1);
                            
                            if (checkMe < 0 || checkMe2 < 0) {
                                fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                                exit(1);
                            }
                    
                    } else {
                        ssize_t checkMe = write(STDOUT_FILENO, (buffer + k), 1);
                        ssize_t checkMe2 = write(pipeToChild[1], (buffer + k), 1);
                        
                        // Check for errors
                        if (checkMe < 0 || checkMe2 < 0) {
                            fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                    }
                }

            }
            
            // If there's an error, react accordingly
            else if (fds[0].revents == POLLERR) {
                fprintf(stderr, "An error with polling has occured with keybord stdin:%s\n", strerror(errno));
                exit(1);
            }

            // Check if the shell has any input in shell
            if (fds[1].revents == POLLIN) {
                // Create buffer and read into it. Check for errors
                char buffer[256];
                ssize_t bytesRead = read(pipeToParent[0], &buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from parent pipe:%s\n", strerror(errno));
                    exit(1);
                }
                
                // Set up for loop to read from shell
                int k, j, counter = 0;
                for (k = 0, j = 0; k < bytesRead; k++) {
                    char currChar = buffer[k];
                    
                    //EOF from shell
                    if (currChar == EOT) {
                        exitFlag = 1;
                    } else if (currChar == NL) {
                        ssize_t checkMe = write(STDOUT_FILENO, (buffer + j), counter);
                        ssize_t checkMe2 = write(STDOUT_FILENO, bunched, 2);
                        
                        // Check for errors
                        if (checkMe < 0 || checkMe2 < 0) {
                            fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                        
                        // Set up variables for use later. Resets counter for next iteration
                        j += (counter + 1);
                        counter = 0;
                        continue;
                    }
                    
                    counter++;
                }
                
                // Write out all the excess bytes. Check for errors.
                ssize_t checkMe = write(STDOUT_FILENO, (buffer+j), counter);
                if (checkMe < 0) {
                    fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                    exit(1);
                }

                // If an error has occured, react accordingly
            } else if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP) {
                exitFlag = 1;
            }
        }
    }
    
    // Close the appropriate pipes and exit. Check Shell exit status beforehand
    close(pipeToParent[0]);
    close(pipeToChild[1]);
    checkShellStatus();
    exit(0);
}

void checkShellStatus(void) {
    // Wait for the child process to finish up. If the return value is
    // negative, then there was an error. childStatus is needed for waitpid
    int childStatus;
    if (waitpid(pid, &childStatus, 0) < 0) {
        fprintf(stderr, "Error with waiting for child process: %s\n", strerror(errno));
        exit(1);
    }
    
    // Report child exit signal and status
    int exitSignal = WTERMSIG(childStatus);
    int exitStatus = WEXITSTATUS(childStatus);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", exitSignal, exitStatus);
}

void handleSigpipe(int val) {
    // val must be used to prevent errors from occuring during build
    if (val == SIGPIPE) {
        // Close appropriate pipes
        close(pipeToChild[1]);
        close(pipeToParent[0]);
        
        // Make sure SIGINT signal is properly sent
        if (kill(pid, SIGINT) < 0) {
            fprintf(stderr, "An error with sending a SIGINT: %s\n", strerror(errno));
            exit(1);
        }
        
        // Output shell status and exit
        checkShellStatus();
        exit(0);
    }
}

