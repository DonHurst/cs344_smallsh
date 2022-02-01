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
        
        // // IF the token is &
        // else if (!strcmp(token, "&")) {
        //     continue;
        // }

        // If the token is a command
        else {
            printf("PRE - %s\n", token);
            // strcpy(currCommand->commandList, token);
            strcat(currCommand->commandList, token);
            // token = strtok(NULL, " ");
            printf("POST - %s\n", token);
            // currCommand->outputFile = calloc(strlen(token) + 1, sizeof(char));
            
        }

        token = strtok(NULL, " ");

        

        // printf("Token: %s\n", token);
        
    }

    printf("inFile - %s\noutFile - %s\nCommands -  %s\n", currCommand->inputFile, currCommand->outputFile, currCommand->commandList);

    return currCommand;

}

int main() {

    int i = 0;
    do {

    struct command *newCommand = getCommand();
    printf("inFile - %s\noutFile - %s\nCommands -  %s\n", newCommand->inputFile, newCommand->outputFile, newCommand->commandList);
    i++;
    } while(i < 5);
}

