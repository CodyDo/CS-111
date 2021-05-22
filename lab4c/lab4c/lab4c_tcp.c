// NAME: Cody Do
// EMAIL: D.Codyx@gmail.com
// ID: 105140467

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mraa.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

// Global variables to make life easier. Initializes them with the default values.
// Can be changed via command line parsing
int period = 1;
char scale = 'F';
int logFile = -1;
int logFlag = 0;
int stop = 0;
char *inputLine = NULL;
time_t nextReadingTime = 0;
struct timeval timer;

int id = 0;
int port = -1;
int sockFD = 0;
char* host="";

mraa_aio_context tempSensor;

// Function prototypes
void handleShutdown(void* a);
void handleTemperature(void);
void handleInput(void);
void handlePrint(char* stringToOutput, int toServ);
float getTemp(void);

int main(int argc, char * argv[]) {
    // Option parsing
    struct option args[] = {
        {"period", required_argument, NULL, 'p'},
        {"scale", required_argument, NULL, 's'},
        {"log", required_argument, NULL, 'l'},
        {"id", required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };
    
    // val used to hold return value of getopt_long. idString used to check id.
    int val;
    char* idString="";
    
    // Parse through the options and responds as necessary
    while((val = getopt_long(argc, argv, "", args, NULL)) != -1) {
        switch(val) {
            case 'p': // period
                period = atoi(optarg);
                break;
                
            case 's': // scale
                // Check to make sure F or C was passed into scale
                if (*optarg != 'F' && *optarg != 'C') {
                    fprintf(stderr, "Error: Scale option only accepts 'F' or 'C'\n");
                    exit(1);
                }
                
                // If F or C was passed, then change the scale
                else {
                    scale = *optarg;
                }
                break;
            
            case 'l': // log
                // Check that an argument was passed
                if (optarg == NULL) {
                    fprintf(stderr, "Error: Log option requires filename\n");
                    exit(1);
                }
                
                // Access or create the input log file. Set logFlag.
                logFile = creat(optarg, S_IRWXU);
                logFlag = 1;
                break;
            
            case 'i': // id
                idString = optarg;
                break;
            
            case 'h':
                host = optarg;
                break;
                
            default: // Bad argument
                fprintf(stderr, "Usage: lab4c [--id=#][--host==NAME][--period=#] [--scale=F,C] [--log=FILENAME]\n");
                exit(1);
        }
    }
    
    // Check that --id, --log, and --host were all specified and used. Make sure
    // they're valid.
    // Check that id is 9 digits
    long leng = strlen(idString);
    if (leng != 9) {
        fprintf(stderr, "Error: Mandatory id must be 9 digits\n");
        exit(1);
    }
    else {
        int a;
        for (a = 0; a < (int) leng; a++) {
            if (!isdigit(idString[a])) {
                fprintf(stderr, "Error: id must be 9 digits\n");
                exit(1);
            }
        }
    }
    id = atoi(idString);
    
    // Check --log
    if (logFlag == 0) {
        fprintf(stderr, "Error: logfile is required\n");
        exit(1);
    }
    
    // Check --host
    if (strlen(host) == 0) {
         fprintf(stderr, "Error: host is required\n");
         exit(1);
    }
    
    // Initialize the sensor and check port.
    tempSensor = mraa_aio_init(1);
    port = atoi(argv[optind]);
    
    if (tempSensor == NULL) {
        fprintf(stderr, "Error initializing temperature sensor\n");
        exit(2);
    }
    if (port == -1) {
        fprintf(stderr, "Error initializing port\n");
        exit(2);
    }
    
    // Create and check socket
    sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        fprintf(stderr, "Error initializing socket\n");
        exit(2);
    }
    
    // Handle host and set-up port
    struct sockaddr_in serverAddy;
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error: Unable to find host\n");
        exit(2);
    }
    
    memset(&serverAddy, 0, sizeof(struct sockaddr_in));
    serverAddy.sin_family = AF_INET;
    memcpy(&serverAddy.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddy.sin_port = htons(port);
    if (connect(sockFD, (struct sockaddr*) &serverAddy, sizeof(serverAddy)) < 0 ) {
        fprintf(stderr, "Error: Failed to connect\n");
        exit(2);
    }
    
    // Send out the ID immediately
    char idOut[50];
    sprintf(idOut, "ID=%d", id);
    handlePrint(idOut, 1);
    
    // Set up poll and prepare variables for the loop. Time variables
    // used to ensure temperature readings are output at period seconds
    struct pollfd pollfd[1];
    pollfd[0].fd = sockFD;
    pollfd[0].events = POLLIN | POLLHUP | POLLERR;
    
    // Prepare inputLine for use in the while loop. MUST memset to 0 or
    // weird lines will appear during the first command read
    inputLine = (char*) malloc(256);
    if (inputLine == NULL) {
        fprintf(stderr, "Error with malloc when reading stdin: %s\n", strerror(errno));
        exit(2);
    }
    memset(inputLine, 0, 256);
    
    // Main loop to handle temperature readings + logging (if needed)
    while (1) {
        // Update timer
        gettimeofday(&timer, 0);
        
        // Handle temperature reading & ensures that it's only output
        // after a period as passed
        if (!stop && (timer.tv_sec >= nextReadingTime))
            handleTemperature();
        
        // Check polling. If it's okay, proceed
        if (poll(pollfd, 1, 1000) < 0) {
            fprintf(stderr, "Error with polling: %s\n", strerror(errno));
            exit(2);
        }
        
        // If there is pending input
        if (pollfd[0].revents & POLLIN) {
            // Read from stdin and check for errors
            char buf[256];
            memset(buf, 0, 256);
            long bytesRead = read(sockFD, &buf, 256);
            if (bytesRead < 0) {
                fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
                exit(2);
            }
            
            // Append each element of buf to inputLine until a newline is encountered
            int j, index = 0;
            for (j = 0; j < bytesRead; j++) {
                // If it's NOT the newline, add it to inputLine
                if (buf[j] != '\n') {
                    inputLine[index] = buf[j];
                    index++;
                }
                
                // If it IS the newline, process the input command
                else {
                    handleInput();
                    memset(inputLine, 0, 256);
                    index = 0;
                }
            }
        }
    }
    
    // Code should never reach here. But just in case.
    mraa_aio_close(tempSensor);
    close(logFile);
    close(sockFD);
    exit(0);
}

void handleTemperature() {
    // Get the current time & temperature. Check for errors.
    struct timespec ts;
    struct tm * tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&(ts.tv_sec));
    float temp = getTemp();
    if (temp == -1) {
        fprintf(stderr, "Error: getTemp returned -1, invalid scale.\n");
        exit(2);
    }
    
    // Output the time/temp + log if needed
    char outputMsg[50];
    sprintf(outputMsg, "%02d:%02d:%02d %0.1f", tm->tm_hour, tm->tm_min, tm->tm_sec, temp);
    handlePrint(outputMsg, 1);
    
    // Increment next reading time
    nextReadingTime = timer.tv_sec + period;
}

void handleInput() {
    // Handle SCALE=F
    if (strcmp(inputLine, "SCALE=F") == 0) {
        scale = 'F';
        handlePrint(inputLine, 0);
    }
    
    // Handle SCALE=C
    else if (strcmp(inputLine, "SCALE=C") == 0) {
        scale = 'C';
        handlePrint(inputLine, 0);
    }
    
    // Handle PERIOD=#
    else if (strncmp(inputLine, "PERIOD=", 7) == 0) {
        // Move forward 7 so that the pointer now looks at everything past PERIOD=
        char *newPeriod = inputLine + 7;
        long length = strlen(newPeriod);
        
        // Check to make sure all the values after PERIOD= are valid digits
        int k;
        for (k = 0; k < (int) length; k++) {
            if (!isdigit(newPeriod[k])) {
                fprintf(stderr, "Error: Invalid Period.\n");
                return;
            }
        }
        
        // Now that we checked, set period to newPeriod
        period = atoi(newPeriod);
        
        // Print out inputline
        handlePrint(inputLine, 0);
    }
    
    // Handle STOP
    else if (strcmp(inputLine, "STOP") == 0) {
        stop = 1;
        handlePrint(inputLine, 0);
    }
    
    // Handle START
    else if (strcmp(inputLine, "START") == 0) {
        stop = 0;
        handlePrint(inputLine, 0);
    }
    
    // Handle LOG
    else if (strncmp(inputLine, "LOG", 3) == 0) {
        if (!logFlag)
            fprintf(stderr, "Log file was not specified on initial run\n");
        else
            handlePrint(inputLine, 0);
    }
    
    // Handle OFF
    else if (strcmp(inputLine, "OFF") == 0) {
        handlePrint(inputLine, 0);
        handleShutdown(NULL);
    }
    
    // Invalid command
    else {
        fprintf(stderr, "Invalid command. Available commands are SCALE=F, SCALE=C, PERIOD=#, STOP, START, LOG, and OFF\n");
        handlePrint(inputLine, 0);
    }
}

void handlePrint(char* stringToOutput, int toServ) {
    // If we want to send to server | 1 = to server, 0 = NOT to stdout
    if (toServ)
        dprintf(sockFD, "%s\n", stringToOutput);
    
    // Logging
    dprintf(logFile, "%s\n", stringToOutput);
}

void handleShutdown(void* a) {
    // If statement is merely here to get rid of the annoying error saying
    // the variable "a" is not used
    if (a) {}
    
    // Get the current time
    struct timespec ts;
    struct tm * tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm = localtime(&(ts.tv_sec));
    
    // Output the time + log if needed
    char outputMsg[50];
    sprintf(outputMsg, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
    handlePrint(outputMsg, 1);
    
    // Close sensor, log file, and sockFD. If inputLine is not NULL then free.
    // Exit once done
    mraa_aio_close(tempSensor);
    close(logFile);
    close(sockFD);
    if (inputLine != NULL)
        free(inputLine);
    exit(0);
}

float getTemp() {
    int reading = mraa_aio_read(tempSensor);
    float thermistorVal = 4275;
    float R_naught = 100000.0;
    
    float R = 1023.0/((float) reading) - 1.0;
    R *= R_naught;
    
    float C = 1.0/(log(R/R_naught)/thermistorVal + 1/298.15) - 273.15;
    
    // Check if we want Celsius or Fahrenheit
    if (scale == 'C')
        return C;
    if (scale == 'F')
        return (C * 9)/5 + 32;
    
    // Return -1 if invalid Scale
    return -1;
}

