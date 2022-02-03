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

#define MAX_LENGTH 2048
#define MAX_ARGS 256

// Function prototypes
struct command *getCommand(); 
void expand(struct command *currCommand, int pidnum);
void executeCommand(struct command *currCommand);
void otherCommands(struct command *currCommand);

// Global Variables
pid_t spawnpid = -5;
int childStatus;
int statusCode;


/********************************************************************************
The command structure holds the list of commands, input file, output file, and 
background status from the user
********************************************************************************/
struct command {
    char* commandList;
    char* inputFile;
    char* outputFile;
    int backgroundStatus;
};

/********************************************************************************
This function takes a command from the user and parses it to create tokens.
It uses those tokens to populate a command (Struct) and return it.
********************************************************************************/
struct command *getCommand() {

    // The general syntax of a command line is:
    // command [arg1 arg2 ...] [< input_file] [> output_file] [&]    

    // Allocate memory for the command
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

    printf("\nRight here - %s\n", currCommand->commandList);

    // printf("The New String - %s", newString);

}

void executeCommand(struct command *currCommand) {

    // Set variable for file descriptor
    int file_descriptor;
    bool redirect = false;
    int std = 2;
    // char* commands[MAX_ARGS];
    int counter = 0;

    printf("\nCURRENT COMMAND INFORMATION -\n");
    // printf("Successfully Entered Parent\n");
    printf("--------------------------------------\n");
    printf("Testing command prints in executeCommand\n");
    printf("Commands - %s", currCommand->commandList);
    printf("\nInputFile - %s", currCommand->inputFile);
    printf("\noutPutFile - %s", currCommand->outputFile);
    printf("\n--------------------------------------\n");

    char* token = strtok(currCommand->commandList, " ");



    while(token) {
        // Set first command in the list to the token & update counter

        printf("In while loop");
        printf("Token - %s", token);
        // commands[counter] = token;
        counter += 1;

        // Advance to next command
        token = strtok(NULL, " ");
    }

    printf("HELLO!");

    while (counter >= 0) {

        // If there is an input file (< redirect was present)
        if (currCommand->inputFile != NULL) {
            file_descriptor = open(currCommand->inputFile, O_RDONLY, 0);
            // IF it opens
            if (file_descriptor) {
                redirect = true;
                std = 0;
            }
            // If it cannot open
            else {
                printf("Cannot open %s input file\n", currCommand->inputFile);
                fflush(stdout);
                exit(1);
            }

        }

        // If there is an output file (> redirect was present)
        else if (currCommand->outputFile != NULL) {
            file_descriptor = open(currCommand->inputFile, O_CREAT | O_WRONLY, 0);
            if (file_descriptor) {
                redirect = true;
                std = 1;
            }
            // If it cannot open
            else {
                printf("Cannot open %s input file\n", currCommand->inputFile);
            }
        }

        if (redirect == true) {
            dup2(file_descriptor, std);
            
        }

        // printf("Commands list command - %s\n", commands[0]);
        // execvp(commands[0], commands);
        close(file_descriptor);
        redirect = false;
        std = 2;
        

    }
    printf("Finished in the execute command\n");
    fflush(stdout);
}

/********************************************************************************


********************************************************************************/
void otherCommands(struct command *currCommand) {

    // The fork code below is adapted directly from the example code in our explorations
    // Found here: https://replit.com/@cs344/4forkexamplec#main.c

    // Fork the process to create a child process
    
    spawnpid = fork();
    printf("\nSpawned new PID!: %di\n", spawnpid);

    switch(spawnpid) {
        
        // If there is an error forking, print message and set exit status to 1
        case -1:
            perror("fork() failed!");
            exit(1);
            break;

        // If the fork executed properly (Child)
        case 0:
            // printf("Successfully Entered Child\n");
            // printf("--------------------------------------\n");
            // printf("Testing command prints in child\n");
            // printf("Commands - %s", currCommand->commandList);
            // printf("\nInputFile - %s", currCommand->inputFile);
            // printf("\noutPutFile - %s", currCommand->outputFile);
            // printf("\n--------------------------------------\n");
            executeCommand(currCommand);
            break;

        // IF we are in the parent
        default:
            // Parent waits for child process to finish first
            waitpid(spawnpid, &childStatus, 0);

            // printf("Successfully Entered Parent\n");
            // printf("--------------------------------------\n");
            // printf("Testing command prints in Parent\n");
            // printf("Commands - %s", currCommand->commandList);
            // printf("\nInputFile - %s", currCommand->inputFile);
            // printf("\noutPutFile - %s", currCommand->outputFile);
            // printf("\n--------------------------------------\n");
            // If child exited normally
            if(WIFEXITED(childStatus)) {
                // Set status code to the return value from the child with WEXITSTATUS
                statusCode = WEXITSTATUS(childStatus);
            }
            break;

    }

    printf("Leaving now!");

}


/********************************************************************************
The handler functions below are adapted from the reading in the below:
https://canvas.oregonstate.edu/courses/1884946/pages/exploration-signal-handling-api?module_item_id=21835981
********************************************************************************/
// void handle_SIGINT(int signo) {
//     printf("terminated by %d\n", signo);
//     fflush(stdout);
// }

/********************************************************************************
The handler functions below are adapted from the reading in the below:
https://canvas.oregonstate.edu/courses/1884946/pages/exploration-signal-handling-api?module_item_id=21835981
********************************************************************************/
// void handle_SIGTSTP(int signo) {
//     if() {

//     }
//     else {

//     }

// }


int main() {

    // Sigint initialization
    // struct sigaction SIGINT_action = {0};
    // struct sigaction SIGTSTP_action = {0};

    




    // Set a variable for the exit status
    int exitStatus = 0;
    char cwd[MAX_LENGTH];

    do {

        printf("In main do loop!\n");

        // Instantiate a new struct and get the input from the user
        struct command *newCommand = getCommand();


        // If the command is NULL (a # was entered), go to next line
        if (newCommand == NULL) {
            continue;
        }

        // Expands any commands with $$
        expand(newCommand, getpid());

        char commandString[MAX_ARGS]; 
        strcpy(commandString, newCommand->commandList);

        // Tokenize our commands
        char* token = strtok(commandString, " ");
        while (token) {

            // If the token says exit, mark indicator flag for exit
            if (strcmp(token, "exit") == 0) {
                exitStatus = 1;
                break;
            }

            // If the token says cd
            else if (strcmp(token, "cd") == 0) {

                printf("\nCurrent Dir (before change) is - %s",getcwd(cwd, sizeof(cwd)));
                
                // Advance to the next token
                token = strtok(NULL, " ");

                // If there is no argument
                if (token == NULL) {
                    chdir(getenv("HOME"));
                    printf("\nCurrent Dir (after change w/ no args) is - %s",getcwd(cwd, sizeof(cwd)));
                }

                // If there is an argument
                else {
                    chdir(token);
                    printf("\nCurrent Dir (after change w/ args) is - %s",getcwd(cwd, sizeof(cwd)));
                }
            }

            // If the token says status
            else if (strcmp(token, "status") == 0) {
                printf("Exit value %d\n", statusCode);
                printf("In status");
                fflush(stdout);
            }

            // If it's any other command
            else {

                printf("--------------------------------------\n");
                printf("Testing command prints in main\n");
                printf("Commands - %s", newCommand->commandList);
                printf("\nInputFile - %s", newCommand->inputFile);
                printf("\noutPutFile - %s", newCommand->outputFile);
                printf("\n--------------------------------------\n");
                otherCommands(newCommand);

            }

            // Advance to next command
            token = strtok(NULL, " ");
        }

    }while (exitStatus != 1);
}


