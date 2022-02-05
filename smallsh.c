#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>

// Constants for the max length and # of args respectively
#define MAX_LENGTH 2048
#define MAX_ARGS 256

// Function prototypes
struct command *getCommand(); 
void expand(struct command *currCommand, int pidnum);
void executeCommand(struct command *currCommand);
void createFork(struct command *currCommand);
void catchSIGTSTP(int signo);
void getStatus(int);

/********************************************************************************
                        * Global Variables *
spawnpid - pid for the forked process
childStatus - for use with waitpid, holds the child's status
processes/numof - array and counter for killing processes on exit 
SIGTSTP_on - indicates if the SIGTSTP is activated (bg processes not allowed)
********************************************************************************/
pid_t spawnpid = -5;
int childStatus;
int statusCode;
int processes[100];
int numOfProcesses = 0;
bool SIGTSTP_on = false;

/********************************************************************************
 *                      * sigaction Structures
Sigaction struct info came directly from Benjamin Brewsters 3.3 signals video:
https://www.youtube.com/watch?v=VwS3dx3uyiQ&list=PL0VYt36OaaJll8G0-0xrqaJW60I-5RXdW&index=18
********************************************************************************/
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};

/********************************************************************************
 *                      * Command Structure *
The command structure holds the list of commands, input file, output file, and 
background status from the user.
********************************************************************************/
struct command {
    char* commandList;
    char* inputFile;
    char* outputFile;
    int backgroundStatus;
};

/********************************************************************************
 *                          * getCommand *
This function takes a command from the user and parses it to create tokens.
It uses those tokens to populate a command (Struct) and return it. It has no 
parameters and uses the getline function to get input from the user. The function
is built to identify redirect values or comment values.
********************************************************************************/
struct command *getCommand() {  

    // Allocate memory for the command
    fflush(stdout);
    struct command *currCommand = malloc(sizeof(struct command));  
    currCommand->commandList = malloc(sizeof(char) * MAX_LENGTH);

    // Instantiate variable for max arguments
    char* args;
    args = (char *)malloc(MAX_LENGTH * sizeof(char));

    // Set the background status to 0 by default
    currCommand->backgroundStatus = 0;

    // Print the : and flush the buffer
    printf(": ");
    fflush(stdout);

    // Create a pointer and length for the getline call
    // char **string_pointer = &args;
    size_t length = 2048;
    
    // Get actual input from the user
    getline(&args, &length, stdin);

    // Remove newline
	for (int i=0; i<MAX_LENGTH; i++) {
		if (args[i] == '\n') {
			args[i] = '\0';
			break;
		}
	}

    // If the line is a comment, return Null
    if (args[0] == '#') {
        return NULL;
    }

    // The following code for tokenizing the individual strings is adapted from a stackoverflow thread 
    // located here: https://stackoverflow.com/questions/266357/tokenizing-strings-in-c
    char* token = strtok(args, " ");
    while (token) {

        // // If the token is <, set value to inputFile
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            currCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->inputFile, token);
        }


        // If the token is >, set value to outputFile
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->outputFile, token);
        }

        // If the token is a &, set the flag to indicate as such
        else if (strcmp(token, "&") == 0) {
            token = strtok(NULL, " ");
            currCommand->backgroundStatus = 1;
            continue;
        }

        // If the token is a command, add it to command list
        else {
            strcat(currCommand->commandList, token);
            strcat(currCommand->commandList, " ");
            
        }
        // Advance to the next token
        token = strtok(NULL, " ");         
    }
    return currCommand;
}

/********************************************************************************
 *                      *   expand function    *
The expand function works by taking our command structure and the PID number.
It copies the commands to a new string and tokenizes it before replacing any 
instances of $$ with the passed in PID number before finally copying the 
list of commands back to our structure
********************************************************************************/
void expand(struct command *currCommand, int pidnum) {

    // Variables to help with the expansion
    char temp;
    char nextTemp;
    char newString[MAX_ARGS];
    char pidString[MAX_ARGS];
    int signsFound;

    // Create a string of the pid number for appending
    sprintf(pidString, "%d", pidnum);

    // Tokenize commands string and iterate over it
    char* token = strtok(currCommand->commandList, " ");
    while (token) {

        // Loop over the string
        for (int i = 0; i < strlen(token); i++) {
            // Set the temp value to the current char and reset the signs found value
            temp = token[i];
            signsFound = 0;

            // Nested loop over the string again
            for(int j = 1; j < strlen(token); j++) {
                // Set the next temp value to the current character in the inner loop
                nextTemp = token[j];

                // If the current and next value are both $, mark the signs found variable
                if ((temp == '$') && (nextTemp == '$') && (j-i == 1)) {
                    signsFound = 1;
                }
            }
            // If we've found a pair of signs, replace with the pidString and advance the iterator
            if (signsFound == 1) {
                strcat(newString, pidString);
                i += 1;
            }

            // If not, just add the current character to our string
            else {
                strncat(newString, &token[i], 1);
            }         
        }
        // Concatenate an empty space to break up the commands
        strcat(newString, " ");

        // Advance to next command
        token = strtok(NULL, " ");
    }
    // Copy the newly created string commands to the structure
    strcpy(currCommand->commandList, newString);
}

/********************************************************************************
 *                          *   execute function    *
The execute command takes our structure as a parameter. It works by looping through
the list of commands and copying them to a new array. It then checks for redirects
before calling exec to run the command.
********************************************************************************/
void executeCommand(struct command *currCommand) {

    // Set variable for file descriptor
    int target_file_descriptor;
    int source_file_descriptor;
    char* commands[MAX_ARGS];
    int counter = 0;

    // Create token for all the commands in the list of commands
    char* token = strtok(currCommand->commandList, " ");

    // While there are still commands to process
    while(token) {

        // Set first command in the list to the token & update counter
        commands[counter] = token;

        // printf("Commands[counter] - %s", commands[counter]);
        counter += 1;

        // Advance to next command
        token = strtok(NULL, " ");
    }

    // Set the final index of commands to null
    commands[counter] = NULL;

    // The code for redirection was adapted from Benjamin Brewster's lecture 3.4
    // https://www.youtube.com/watch?v=9Gsp-wucTNw&list=PL0VYt36OaaJll8G0-0xrqaJW60I-5RXdW&index=19

    // If there is an input file (< redirect was present)
    if (currCommand->inputFile != NULL) {
        
        // Attempt to open the file
        source_file_descriptor = open(currCommand->inputFile, O_RDONLY);
       // If we cannot open the input file
        if (source_file_descriptor == -1) {
            printf("Cannot open %s for input\n", currCommand->inputFile);
            exit(1);
        }
        else {
            // redirect the input to be from the indicated file
            dup2(source_file_descriptor, 0);
        }
        // Close the file descriptor
        close(source_file_descriptor);

    }

    // If there is an output file (> redirect was present)
    if (currCommand->outputFile != NULL) {
        
        // Open or create output file
        target_file_descriptor = open(currCommand->outputFile, O_CREAT | O_WRONLY | O_TRUNC, 0640);
        if (target_file_descriptor == -1) {
            perror("open()");
            exit(1);
        }
        else {
            // redirect the output to be sent to the target file
            dup2(target_file_descriptor, 1);
        }
        // Close the file descriptor
        close(target_file_descriptor);
        
    }

    // IF this is a foreground process
    if (currCommand->backgroundStatus == 0) {
        // Set action to default functionality (closes on ctrl+c)
        SIGINT_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &SIGINT_action, NULL);
    }

    // Execute the command
    if((statusCode = execvp(commands[0], commands)) == -1) {
        // IF it cannot execute, print error
        perror(commands[0]);
        exit(statusCode);
    }
    
    // Flush the std out buffer
    fflush(stdout);
}

/********************************************************************************
 *                          *   createFork function    *
The create fork command takes our structure as a parameter. It works by forking
the process before executing the command  (calling execute command). The parent
process will wait/not wait based on the background status flag in our struct.
finally, the parent process will reap any background children when they are 
done
********************************************************************************/
void createFork(struct command *currCommand) {

    // The fork code below is adapted directly from the example code in our explorations
    // Found here: https://replit.com/@cs344/4forkexamplec#main.c

    // Fork the process to create a child process
    spawnpid = fork();

    // Store the process id and increment process counter
    processes[numOfProcesses] = spawnpid;
    numOfProcesses += 1;

    switch(spawnpid) {
        
        // If there is an error forking, print message and set exit status to 1
        case -1:
            perror("fork() failed!");
            exit(1);
            break;

        // If the fork executed properly (Child)
        case 0:
            // Execute the current command   
            executeCommand(currCommand);
            break;

        // If we are in the parent
        default:
            // If the process is a background command
            if(currCommand->backgroundStatus == 1 && SIGTSTP_on == false) {
                // Process returns immediately and prints the pid
                waitpid(spawnpid, &childStatus, WNOHANG);
                printf("The background pid is starting - %d\n", spawnpid);
                fflush(stdout);
            }

            // If it is a foreground process
            else {
                // Process runs and parent waits for it to finish
                waitpid(spawnpid, &childStatus, 0);
            }
            
        // This loop was adapted from stack overflow:
        // https://stackoverflow.com/questions/45809535/why-does-waitpid-in-a-signal-handler-need-to-loop
        
        // Reap the background child processes when they are done
        while((spawnpid = waitpid(-1, &childStatus, WNOHANG)) > 0) {
            printf("Background pid %d is done: ", spawnpid);
            getStatus(childStatus);
            fflush(stdout);
            
        }
        fflush(stdout);
    }

}

/********************************************************************************
 *                      *  getStatus function   *
The getStatus function takes only an int representing the child's status as an
argument and returns nothing. It simply checks and prints the exit status.
Adapted/referenced from: https://www.geeksforgeeks.org/exit-status-child-process-linux/
********************************************************************************/
void getStatus(int childStatus) {
    
    // Check the child's termination status
    if (WIFEXITED(childStatus)) {
        // If it exited normally
        printf("exit value %d\n", WEXITSTATUS(childStatus));
    } 
    else {
        // If exited via signal
        printf("exit value: %d\n", WTERMSIG(childStatus));
    }

}

/********************************************************************************
 *                      *  catchSIGTSTP function   *
 * 
 * 
Adapted from Benjamin Brewster 3.3 - Signals video:
https://www.youtube.com/watch?v=VwS3dx3uyiQ
********************************************************************************/
void catchSIGTSTP(int signo) {

    // If the SIGTSTP signal is not on (bg process allowed)
    if (SIGTSTP_on == false) {
        // Set message and write using non-reentrant function
        char* msg = "Entering foreground-only mode (& is now ignored)\n";
        write(1, msg, 50);

        // Turn the SIGTSTP flag on (bg process not allowed)
        SIGTSTP_on = true;

    }

    // If the SIGTSTP signal is on (BG process not allowed)
    else {
        // Set message and write using non-reentrant function
        char* msg = "Exiting foreground-only mode\n";
        write(1, msg, 30);

        // Turn off the SIGTSTP flag (bg process allowed)
        SIGTSTP_on = false;

    }

}


int main() {

    //              SIGINT
    // Ignore instances of SIGINT by default
    SIGINT_action.sa_handler = SIG_IGN;
    // Block signals while mask in place
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    // Register the SIGINT functionality
    sigaction(SIGINT, &SIGINT_action, NULL);

    //              SIGTSTP
    //Set the sa handler to our catch function
    SIGTSTP_action.sa_handler = catchSIGTSTP; 
    // Block signals while mask in place
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    // Register the SIGTSTP functionality
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    // Set a variable for the exit status
    int exitStatus = 0;
    int builtIn = 0;

    do {

        // Instantiate a token to check if this is the first command in the list
        int firstToken = 0;
        
        // Instantiate a new struct and get the input from the user
        struct command *newCommand = getCommand();


        // If the command is NULL (a # was entered), go to next line
        if (newCommand == NULL) {
            continue;
        }

        // Expands any commands with $$
        expand(newCommand, getpid());

        // Create new command string and copy contents of commands to it
        char commandString[MAX_ARGS]; 
        strcpy(commandString, newCommand->commandList);

        // Tokenize our commands
        char* token = strtok(commandString, " ");
        while (token) {


            // If the token says exit, mark indicator flag for exit
            if ((strcmp(token, "exit") == 0) && firstToken == 0) {
                // If there are no processes, set exit flag to 0
                if (numOfProcesses == 0){
                    exit(0);
                }
                //If there are, kill them and set flag to 1
                else {
                    int counter;
                    for(counter = 0; counter < numOfProcesses; counter++) {
                        kill(processes[counter], SIGTERM);
                    }
                    exit(1);
                }
                builtIn = 1; 
                break;
            }

            // If the token says cd
            else if ((strcmp(token, "cd") == 0) && firstToken == 0) {

                builtIn = 1;
                
                // Advance to the next token
                token = strtok(NULL, " ");

                // If there is no argument
                if (token == NULL) {
                    chdir(getenv("HOME"));
                }

                // If there is an argument
                else {
                    chdir(token);
                }
            }

            // If the token says status
            else if ((strcmp(token, "status") == 0) && firstToken == 0) {
                builtIn = 1;

                getStatus(childStatus);
                fflush(stdout);

                // printf("Exit value: %d\n", statusCode);
                // fflush(stdout);
            }

            // Advance to next command
            token = strtok(NULL, " ");
            firstToken += 1;
        }
        // When we've processed all the commands (and found the command isn't built-in), 
        // fork the process to execute the command
        if (builtIn != 1) {
            createFork(newCommand);
        }

        builtIn = 0;

    }while (exitStatus != 1);

    return 0;
}


