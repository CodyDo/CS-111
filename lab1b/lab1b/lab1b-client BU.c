// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h> // fprintf()
#include <stdlib.h> // exit()
#include <termios.h> // All the terminal editing functions/structs
#include <errno.h>  // used for errno in error output
#include <string.h> // strerror()
#include <getopt.h> // getopt()
#include <fcntl.h>  // open(), close()
#include <sys/types.h> // Provided by TA Slides; Used for Sockets
#include <sys/socket.h> // Provided by TA Slides; Used for Sockets
#include <sys/wait.h> // Provided by TA Slides; Used for Sockets
#include <netinet/in.h> // Provided by TA Slides; Used for Sockets
#include "zlib.h" // Needed for compression
#include <netdb.h> // Needed for gethostbyname()
#include <poll.h> // For polling functionality

// Stores the original terminal modes for restoration later
struct termios originalState;

// Function prototypes. Description of what it does can be found above implementation
void prepTerminalModes(void);
void restoreTerminalModes(void);
void decompressBuffer(char* bufferToCompress, int count);
void compressBuffer(char* buffer, int socket);

// Global flag for logging so make it easier for helper functions to work
int logFlag;
int logFile;
int exitFlag;

// Establish uniform definitions
#define CR '\015'  // \r
#define NL '\012'  // \n
#define EOT '\004' // 0x04 End of File, not EOF because EOF already existed
#define ITR '\003' // 0x03 Interupt
char bunched[2] = {CR, NL};

int main(int argc, char * argv[]) {
    // Set up basic flags/options. These will be modified later if the approprite
    // options are found
    logFlag = 0;
    logFile = 0;
    int portNum = -1;
    int compressFlag = 0;
    char* logFileName;
    
    // Option parsing (based off setup from lab0)
    struct option args[] = {
        {"log", required_argument, NULL, 'l'},
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
            case 'l': // --log
                // Set the logFlag and save log file name.
                logFlag = 1;
                logFileName = optarg;
                break;
            
            case 'p': // --port
                portNum = atoi(optarg);
                break;
            
            case 'c': // --compress
                compressFlag = 1;
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab1b --port=PORTNUMBER [--log=FILENAME] [--compress]\n");
                exit(1);
        }
    }
    
    // If the port number was NOT specified, exit. This is required.
    if (portNum == -1) {
        fprintf(stderr, "Port number required. Proper usage: lab1b --port=PORTNUMBER [--log=FILENAME] [--compress]\n");
        exit(1);
    }
    
    // Create log file is --log was an option
    if (logFlag) {
        logFile = creat(logFileName, 0666);
        
        // Check for errors
        if (logFile < 0) {
            fprintf(stderr, "Error creating/opening log file: %s\n", strerror(errno));
            exit(1);
        }
    }
    
    // Setting up the socket. Heavily based off the slides posted by the TAs
    int sockfd;
    struct sockaddr_in serverAddress;
    struct hostent* server;
    
    // Create socket and check for errors. If error, then exit.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error opening client socket: %s\n", strerror(errno));
        exit(1);
    }
    
    // Fill in the appropriate server information.
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNum);
    server = gethostbyname("localhost");
    
    // Copy IP address from server to serverAddress. Then pad with zeroes
    memcpy(&serverAddress.sin_addr.s_addr, server->h_addr, server->h_length);
    memset(serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));
    
    // Connect with server socket. Check for errors.
    int clientSocketStatus = connect(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if (clientSocketStatus < 0) {
        fprintf(stderr, "Error connecting to server socket: %s\n", strerror(errno));
        exit(1);
    }
    
    // Place terminal into non-canonical input mode with no echo.
    // atexit() will cause the terminal restoration command to run prior to
    // exiting the terminal.
    prepTerminalModes();
    atexit(restoreTerminalModes);
    
    // Prepare polling functionality.
    struct pollfd fds[2] = {
        { STDIN_FILENO, POLLIN, 0},
        { sockfd, POLLIN, 0}
    };
    
    exitFlag = 0;

    while (!exitFlag) {
        // ret holds the number of file descriptors that match the events (POLLIN)
        int ret = poll(fds, 2, 0);
        if (ret > 0) {
            // If there is input from the user/keyboard
            if (fds[0].revents & POLLIN) {
                // Create buffer and read into it. Check for errors
                char* buffer = (char*) malloc(256);
                memset(buffer, 0, 256);
                ssize_t bytesRead = read(STDIN_FILENO, &buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from keyboard:%s\n", strerror(errno));
                    exit(1);
                }
                
                // Loop through everything that is read
                int k;
                for (k = 0; k < bytesRead; k++) {
                    char currChar = buffer[k];
                    
                    // Response to \r or \n
                    if (currChar == CR || currChar == NL) {
                        // Echo to terminal
                        ssize_t checkMe = write(STDOUT_FILENO, &bunched, 2);
                        if (checkMe < 0) {
                            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                        
                        // If compression flag is present, compress newline and
                        // use compressBuffer() function. Else, write it normally.
                        if (compressFlag) {
                            char nl = NL;
                            compressBuffer(&nl, sockfd);
                        }
                        else {
                            ssize_t check1 = write(sockfd, "\n", 1);
                            if (check1 < 0) {
                                fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
                                exit(1);
                            }
                            
                            // If the --log option was used, write to the log!
                            if (logFlag) {
                                // Create a buffer and set up the required text.
                                char SENTbuffer[256];
                                snprintf(SENTbuffer, 256, "SENT %d bytes: ", (int)strlen(SENTbuffer));
                                
                                // Add the SENT... start, write what was sent,
                                // then end with a newline
                                ssize_t check1 = write(logFile, SENTbuffer, 1);
                                ssize_t check2 = write(logFile, &currChar, 1);
                                ssize_t check3 = write(logFile, "\n", 1);
                                
                                if (check1 < 0 || check2 < 0 || check3 < 0) {
                                    fprintf(stderr, "Error writing sent data to log: %s\n", strerror(errno));
                                    exit(1);
                                }
                            }
                        }
                    }
                    
                    // If not \r or \n
                    else {
                        // Echo to terminal
                        ssize_t checkMe = write(STDOUT_FILENO, &currChar, 1);
                        if (checkMe < 0) {
                            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                            exit(1);
                        }
                        
                        // If compression flag is present, compress newline and
                        // use compressBuffer() function. Else, write it normally.
                        if (compressFlag) {
                            compressBuffer(&currChar, sockfd);
                        }
                        else {
                            ssize_t check1 = write(sockfd, &currChar, 1);
                            if (check1 < 0) {
                                fprintf(stderr, "Error writing to socket: %s\n", strerror(errno));
                                exit(1);
                            }
                            
                            // If the --log option was used, write to the log!
                            if (logFlag) {
                                // Create a buffer and set up the required text.
                                char prefix[256];
                                snprintf(prefix, 256, "SENT %d bytes: ", 1);
                                
                                // Add the SENT... start, write what was sent,
                                // then end with a newline
                                ssize_t check1 = write(logFile, prefix, strlen(prefix));
                                ssize_t check2 = write(logFile, &currChar, 1);
                                ssize_t check3 = write(logFile, "\n", 1);
                                
                                if (check1 < 0 || check2 < 0 || check3 < 0) {
                                    fprintf(stderr, "Error writing to log: %s\n", strerror(errno));
                                    exit(1);
                                }
                            }
                        }
                    }
                }
                // Free the buffer before leaving
                // free(buffer);
            }
            
            // If there's an error, react accordingly
            else if (fds[0].revents & POLLERR) {
                fprintf(stderr, "An error with polling has occured with keybord stdin:%s\n", strerror(errno));
                exit(1);
            }

            // Check if the client has any input from server
            if (fds[1].revents & POLLIN) {
                // Create buffer and read into it. Check for errors
                char* buffer = (char*) malloc(256);
                memset(buffer, 0, 256);
                ssize_t bytesRead = read(sockfd, &buffer, 256);
                if (bytesRead < 0) {
                    fprintf(stderr, "Error reading from socket:%s\n", strerror(errno));
                    exit(1);
                } else if (bytesRead == 0) {
                    // bytesRead == 0 during an EOF
                    exitFlag = 1;
                }
                
                // If the --log option was used, write to the log!
                if (logFlag) {
                    // Create a buffer and set up the required text. Cast bytesRead
                    // to in avoid errors
                    char prefix[256];
                    snprintf(prefix, 256, "RECEIVED %d bytes: ", (int)bytesRead);
                    
                    // Add the RECEIVED... start, write what was sent,
                    // then end with a newline
                    ssize_t check1 = write(logFile, prefix, strlen(prefix));
                    ssize_t check2 = write(logFile, buffer, bytesRead);
                    ssize_t check3 = write(logFile, "\n", 1);
                    
                    if (check1 < 0 || check2 < 0 || check3 < 0) {
                        fprintf(stderr, "Error writing received data to log: %s\n", strerror(errno));
                        exit(1);
                    }
                }
                
                // If the compressFlag is toggled, then decompress it as needed.
                // Else, loop and write out what was read.
                if (compressFlag) {
                    decompressBuffer(buffer, (int)bytesRead);
                }
                
                else {
                    int k;
                    for (k = 0; k < bytesRead; k++) {
                        char currChar = buffer[k];
                        
                        // If \r or \n
                        if (currChar == CR || currChar == NL) {
                            ssize_t checkMe = write(STDOUT_FILENO, &bunched, 2);
                            if (checkMe < 0) {
                                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                                exit(1);
                            }
                        }
                        
                        // If not \r or \n
                        else {
                            ssize_t checkMe = write(STDOUT_FILENO, &currChar, 1);
                            if (checkMe < 0) {
                                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                                exit(1);
                            }
                        }
                    }
                }
                

                // Free up the buffer
                // free(buffer);
                
                // If an error has occured, react accordingly
            } else if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP) {
                exitFlag = 1;
            }
        }
    }
    
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

// Handles decompressing any compressed data from the client. Heavily based on the
// TA slides
void decompressBuffer(char* bufferToDecompress, int count) {
    // Initialize strm to pass info to/from zlib routines. Set up inflate state
    z_stream strm;
    char* decompressedBuffer = (char*) malloc(256);
    
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    int checkMe = inflateInit(&strm);
    if (checkMe != Z_OK) {
        fprintf(stderr, "Error using inflateInit() to decompress client data: %s\n", strerror(errno));
        exit(1);
    }
    
    // avail_in = # bytes available in next_in, next_in = next input byte
    // avail_out = Remaining free space at next_out, next_out = next output byte
    // Need (Bytef*) or XCode threw up an error
    strm.avail_in = count;
    strm.next_in = (Bytef*)bufferToDecompress;
    strm.avail_out = 256;
    strm.next_out = (Bytef*)decompressedBuffer;
    
    // While loop to decompress everything
    do {
        inflate(&strm, Z_SYNC_FLUSH);
    } while (strm.avail_in > 0);
    
    // Write freshly decompressed data to the terminal
    // Need (int) or error will appear during make
    int k;
    for (k = 0; k < (int)(256 - strm.avail_out); k++) {
        // If newline, write our \r and \n
        if (decompressedBuffer[k] == '\n') {
            // Write out bunched and check for errors
            ssize_t checkMe2 = write(STDOUT_FILENO, &bunched, 2);
            if (checkMe2 < 0) {
                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                exit(1);
            }
            
        }
        // If not newline, just write out normally.
        else {
            ssize_t checkMe2 = write(STDOUT_FILENO, &decompressedBuffer[k], 1);
            if (checkMe2 < 0) {
                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                exit(1);
            }
        }
    }
    
    // Finished and clean up
    inflateEnd(&strm);
    free(decompressedBuffer);
}

// Handles compressing any data to the client. Heavily based on the
// TA slides. Includes logging functionality already.
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
        exit(1);
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
    } while (strm.avail_in > 0);
    
    // Now that everything has been compressed, write it out to the server. Check for
    // errors.
    ssize_t checkMe2 = write(socket, compressedBuffer, 256 - strm.avail_out);
    if (checkMe2 < 0) {
        fprintf(stderr, "Error writing compressed data from server to client: %s\n", strerror(errno));
        exit(1);
    }
    
    // If the --log option was used, write to the log!
    if (logFlag) {
        // Create a buffer and set up the required text. Casts strlen() into int
        // so no error is given with %d
        char prefix[256];
        snprintf(prefix, 256, "SENT %d bytes: ", (int)strlen(compressedBuffer));
        
        // Add the SENT... start, write what was sent, then end with a newline
        ssize_t check1 = write(logFile, prefix, strlen(prefix));
        ssize_t check2 = write(logFile, compressedBuffer, strlen(compressedBuffer));
        ssize_t check3 = write(logFile, "\n", 1);
        
        if (check1 < 0 || check2 < 0 || check3 < 0) {
            fprintf(stderr, "Error writing to log: %s\n", strerror(errno));
            exit(1);
        }
    }
    
    // Everything went well by this point. Clean up.
    deflateEnd(&strm);
    free(compressedBuffer);
}

