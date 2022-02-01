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

    char temp;
    char nextTemp;
    char newString[MAX_ARGS];
    char pidString[MAX_ARGS];
    int signsFound;

    sprintf(pidString, "%d", pidnum);

    printf("The PID - %s\n", pidString);

            // temp = token[i];
            // nextTemp = token[i]+1;
            

            // printf("The Character - %c\n", temp);
            // printf("The Next Character - %c\n\n", temp);
            
            // printf("Token - %c", token[i]);
            // if ((strcmp(temp,"$") == 0) && strcmp(temp, "$") == 0) {
            //     printf("In here!");
            //     strcat(newString, pidString);
            //     i += 1;
            // }
            // else {
            //     strcat(newString, &temp[i]);
            // }


    // Tokenize commands string and iterate over it
    char* token = strtok(currCommand->commandList, " ");
    while (token) {
        printf("The token! - %s\n", token);
        // Loop over the string
        for (int i = 0; i < strlen(token); i++) {
            temp = token[i];
            signsFound = 0;

            // Nested loop over the string again
            for(int j = 1; j < strlen(token); j++) {
                nextTemp = token[j];

                // If 
                if ((temp == '$') && (nextTemp == '$') && (j-i == 1)) {
                    signsFound = 1;
                }

                // printf("the temp - %c\n", temp);
                // printf("the next temp - %c\n", nextTemp);
                // If the current and next value are both $'s, replace with the pid
                // if ((token[i] == "$") && (strcmp(token[j], "$")) && (j-i == 1)){
                //     printf("They MAtch!");
                // }
            }
            if (signsFound == 1) {
                strcat(newString, pidString);
                i += 1;
            }
            else {
                strncat(newString, &token[i], 1);
            }
            
            
        }
        // Advance to next command
        token = strtok(NULL, " ");
    }

    printf("The New String - %s", newString);

    // printf("\n The Temp - %s", temp);

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


