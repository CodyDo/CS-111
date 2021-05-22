// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h> // fprintf()
#include <stdlib.h> // exit()
#include <getopt.h> // getopt()
#include <errno.h>  // used for errno in error output
#include <string.h> // strerror()
#include <unistd.h> // read(), write()
#include <poll.h> // For polling functionality
#include <signal.h> // kill()
#include <sys/wait.h> //waitpid()
#include <sys/types.h> // Provided by TA Slides; Used for Sockets
#include <sys/socket.h> // Provided by TA Slides; Used for Sockets
#include <sys/wait.h> // Provided by TA Slides; Used for Sockets
#include <netinet/in.h> // Provided by TA Slides; Used for Sockets
#include "zlib.h" // Needed for compression

// Function prototypes. Description of what it does can be found above implementation
void doChildStuff(void);
void doParentStuff(int clientSocket);
void checkShellStatus(void);
void decompressBuffer(char* originalBuffer, char* decompressedBuffer, int count);
void compressBuffer(char* buffer, int socket);

// These are all global variables to allow easy access for helper functions
int pipeToChild[2];
int pipeToParent[2];
pid_t pid;
int compressFlag;

// Establish uniform definitions
#define CR '\015'  // \r
#define NL '\012'  // \n
#define EOT '\004' // 0x04 End of File, not EOF because EOF already existed
#define ITR '\003' // 0x03 Interupt

int main(int argc, char * argv[]) {
    // Set portNum = -1 and compressFlag = 0. These will be changed if the appropriate
    // options are found later.
    int portNum = -1;
    compressFlag = 0;

    // Option parsing
    struct option args[] = {
        {"port", required_argument, NULL, 'p'},
        {"compress", no_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };

    // val used to hold return value of getopt_long
    int val;

    // Parse through the options and see if "--shell" is present. If another option
    // exists, exit with error
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 'p': // --port
                portNum = atoi(optarg);
                break;

            case 'c': // --compress
                compressFlag = 1;
                break;

            default: // Bad argument
                fprintf(stderr, "Usage: lab1b --port=PORTNUMBER [--compress]\n");
                exit(1);
        }
    }

    // If the port number was NOT specified, exit. This is required.
    if (portNum == -1) {
        fprintf(stderr, "Port number required. Proper usage: lab1b --port=PORTNUMBER [--compress]\n");
        exit(1);
    }

    // Setting up the socket. Heavily based off the slides posted by the TAs
    // sockfd = listen using this, new_fd = new connection (end product)
    int sockfd, new_fd;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sin_size;

    // Create socket and check for errors. If error, then exit.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error opening server socket: %s\n", strerror(errno));
        exit(1);
    }

    // Set up all the address information & pad with zeroes:
    // AF_INET = Proper address family, htons(portNum) = Proper Network Byte Order,
    // INADDR_ANY = Allows client to connect to any of the host's IP Addresses
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNum);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));

    // Binding the socket to the IP address and port number. Check for errors.
    int bindValue = bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr));
    if (bindValue < 0) {
        fprintf(stderr, "Error binding server socket: %s\n", strerror(errno));
        exit(1);
    }

    // Listen for the client and accept any new connection
    listen(sockfd, 5);
    sin_size = sizeof(struct sockaddr_in);
    new_fd = accept(sockfd, (struct sockaddr*)&clientAddress, &sin_size);
    if (new_fd < 0) {
        fprintf(stderr, "Error accepting new connection from client: %s\n", strerror(errno));
        exit(1);
    }


    // Taken from lab 1a. Do the same procedure as if --shell were present in lab 1a
    // Create pipes and react accordingly.
    /*             ------> pipeToChild ----->
        [Parent]                                [Child Shell]
                   <----- pipeToParent <-----
                                                                */
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
        doParentStuff(new_fd);
    }

    // Close socket and exit! Check for errors.
    if (close(sockfd) < 0) {
        fprintf(stderr, "Error closing socket: %s\n", strerror(errno));
        exit(1);
    }
    exit(0);
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
void doParentStuff(int clientSocket) {
    // Close unnecessary pipes (right side of the diagram in week 2 disc slides)
    close(pipeToChild[0]);
    close(pipeToParent[1]);


    // Prepare polling functionality.
    struct pollfd fds[2] = {
        {clientSocket, POLLIN, 0},
        {pipeToParent[0], POLLIN, 0}
    };

    char* buffer = (char*) malloc(256);
    int bytesRead = 0;

    while (1) {
        // ret holds the number of file descriptors that match the events (POLLIN)
        int ret = poll(fds, 2, 0);
        if (ret > 0) {
            // check that stdin has pending input
            if (fds[0].revents & POLLIN) {
                // Create buffer and read into it. Check for errors
                memset(buffer, 0, 256);
                bytesRead = read(clientSocket, buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from client socker:%s\n", strerror(errno));
                    exit(1);
                }

                // If compression flag is present, then decompress what was just read
                if (compressFlag) {
                    char* decompressedBuffer = (char*) malloc(256);
                    memset(decompressedBuffer, 0, 256);
                    decompressBuffer(buffer, decompressedBuffer, bytesRead);
                    memset(buffer, 0, 256);
                    buffer = decompressedBuffer;
                }

                // Loop through everything that is read
                int k;
                for (k = 0; k < bytesRead; k++) {
                    // Response to EOT (\004)
                    if (buffer[k] == EOT) {
                        if (compressFlag) {
                            char* sendMe = "^D";
                            compressBuffer(sendMe, clientSocket);
                        }
                        else {
                            ssize_t checkMe = write(clientSocket, "^D", 2);
                            if (checkMe < 0) {
                                fprintf(stderr, "Error writing to stdout:%s\n", strerror(errno));
                                exit(1);
                            }
                        }
                        
                        close(pipeToChild[1]);

                    // Response to an interupt (\003)
                    } else if (buffer[k] == ITR) {
                        if (compressFlag) {
                            char * sendMe = "^C";
                            compressBuffer(sendMe, clientSocket);
                        }
                        else {
                            ssize_t checkMe = write(clientSocket, "^C", 2);
                            if (checkMe < 0) {
                                fprintf(stderr, "Error writing to stdout:%s\n", strerror(errno));
                                exit(1);
                            }
                        }
                        if (kill(pid, SIGINT) < 0) {
                            fprintf(stderr, "An error with sending a SIGINT: %s\n", strerror(errno));
                            exit(1);
                        }

                    }  else {
                        ssize_t checkMe = write(pipeToChild[1], &buffer[k], 1);

                        // Check for errors
                        if (checkMe < 0) {
                            fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                    }
                }
            }

            // If there's an error, react accordingly
            else if (fds[0].revents & POLLERR) {
                fprintf(stderr, "An error with polling has occured with keybord stdin:%s\n", strerror(errno));
                exit(1);
            }

            // Check if the shell has any input in shell
            else if (fds[1].revents & POLLIN) {
                // Read into buffer. Check for errors
                memset(buffer, 0, 256);
                bytesRead = read(pipeToParent[0], buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from parent pipe:%s\n", strerror(errno));
                    exit(1);
                } else if (bytesRead == 0) {
                    // bytesRead == 0 during an EOF
                    checkShellStatus();
                    break;
                }

                // Set up for loop to read from shell. For each character, write to the client
                // socket. If the compressFlag is toggled, then compress it as needed.
                int k;
                for (k = 0; k < bytesRead; k++) {
                    if (compressFlag)
                        compressBuffer(&buffer[k], clientSocket);
                    else {
                        ssize_t checkMe = write(clientSocket, &buffer[k], 1);
                        if (checkMe < 0 ) {
                            fprintf(stderr, "An error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                    }
                }
            }
            // If an error has occured, react accordingly
            else if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP) {
                checkShellStatus();
                break;
            }
        }
        else if (ret < 0) {
            fprintf(stderr, "Error with polling: %s\n", strerror(errno));
            exit(1);
        }
    }
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

// Handles decompressing any compressed data from the client. Heavily based on the
// TA slides
void decompressBuffer(char* originalBuffer, char* decompressedBuffer, int count) {
    // Initialize strm to pass info to/from zlib routines. Set up inflate state
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    int checkMe = inflateInit(&strm);
    if (checkMe != Z_OK) {
        fprintf(stderr, "Error using inflateInit() to decompress client data: %s\n", strerror(errno));
    }

    // avail_in = # bytes available in next_in, next_in = next input byte
    // avail_out = Remaining free space at next_out, next_out = next output byte
    // Need (Bytef*) or XCode threw up an error
    strm.avail_in = count;
    strm.next_in = (Bytef*)originalBuffer;
    strm.avail_out = 256;
    strm.next_out = (Bytef*)decompressedBuffer;

    // While loop to decompress everything
    do {
        inflate(&strm, Z_SYNC_FLUSH);
    }while(strm.avail_in > 0);

    // Finished and clean up
    inflateEnd(&strm);
}

// Handles compressing any data to the client. Heavily based on the
// TA slides
void compressBuffer(char* bufferToCompress, int socket) {
    // Initialize strm to pass info to/from zlib routines. Set up deflate state
    z_stream strm;
    char* compressedBuffer = (char*) malloc(256);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    int checkMe = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (checkMe != Z_OK) {
        fprintf(stderr, "Error using deflateInit() to compress server data: %s\n", strerror(errno));
    }

    // avail_in = # bytes available in next_in, next_in = next input byte
    // avail_out = Remaining free space at next_out, next_out = next output byte
    // Need (Bytef*) or XCode threw up an error
    strm.avail_in = 1;
    strm.next_in = (Bytef*)bufferToCompress;
    strm.avail_out = 256;
    strm.next_out = (Bytef*)compressedBuffer;

    // While loop to compress everything
    do {
        deflate(&strm, Z_SYNC_FLUSH);
    }while(strm.avail_in > 0);

    // Now that everything has been compressed, write it out to the client. Check for
    // errors.
    ssize_t checkMe2 = write(socket, compressedBuffer, 256 - strm.avail_out);
    if (checkMe2 < 0) {
        fprintf(stderr, "Error writing compressed data from server to client: %s\n", strerror(errno));
        exit(1);
    }

    // Everything went well by this point. Clean up.
    deflateEnd(&strm);
    free(compressedBuffer);
}
