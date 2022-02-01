#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_LENGTH 2048
#define MAX_ARGS 256


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

    // printf("You typed: %s\n", args);

    // The following code for tokenizing the individual strings is adapted from a stackoverflow thread 
    // located here: https://stackoverflow.com/questions/266357/tokenizing-strings-in-c
    char* token = strtok(args, " ");
    while (token) {

        printf("Token: %s\n", token);

        // // If the token is <
        if (strcmp(token, "<") == 0) {
            printf("PRE - %s\n", token);
            token = strtok(NULL, " ");
            printf("POST - %s\n", token);
            currCommand->inputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->inputFile, token);
        }


        // // If the token is >
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->outputFile, token);
        }

        else if (strcmp(token, "&") == 0) {
            token = strtok(NULL, " ");
            currCommand->backgroundStatus = 1;
            continue;
        }
        
        // // IF the token is &
        // else if (!strcmp(token, "&")) {
        //     continue;
        // }

        // If the token is a command
        else {
            printf("PRE - %s\n", token);
            // strcpy(currCommand->commandList, token);
            strcat(currCommand->commandList, token);
            strcat(currCommand->commandList, " ");
            // token = strtok(NULL, " ");
            printf("POST - %s\n", token);
            // currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
            
        }

        token = strtok(NULL, " ");

        

        // printf("Token: %s\n", token);
        
    }

    // printf("inFile - %s\noutFile - %s\nCommands -  %s\nBackgroundStatus - %d", currCommand->inputFile, currCommand->outputFile, currCommand->commandList, currCommand->backgroundStatus);

    return currCommand;

}

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
        // Advance to next command
        token = strtok(NULL, " ");
    }

    printf("The New String - %s", newString);

}

int main() {

    // int i = 0;

    printf("%d", getpid());
    // do {

    struct command *newCommand = getCommand();
    // printf("inFile - %s\noutFile - %s\nCommands -  %s\nBackgroundStatus - %d", newCommand->inputFile, newCommand->outputFile, newCommand->commandList, newCommand->backgroundStatus);
    // i++;
    // } while(i < 5);
    printf("\n The PID - %d", getpid());
    expand(newCommand, getpid());
}


