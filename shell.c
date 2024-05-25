// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)


/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if (length < 0) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}

/**
 * helper function for printing text
 */
void print_string(char text[])
{
	write(STDOUT_FILENO, text, strlen(text));
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
    // while (waitpid(-1, NULL, WNOHANG) > 0);
    
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];

	char homeDir[PATH_MAX];
	snprintf(homeDir, sizeof(homeDir), "/home/%s", getenv("USER"));
	chdir(homeDir);

	while (true) {

		char *cwd = (char*) malloc(sizeof(char) * PATH_MAX);

		if (getcwd(cwd, sizeof(char) * PATH_MAX) != NULL){
			print_string(cwd);
        } else {
            perror("getcwd() error");
        }

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		print_string("$ ");
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);

		// DEBUG: Dump out arguments:
		// for (int i = 0; tokens[i] != NULL; i++) {
		// 	write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
		// 	write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
		// 	write(STDOUT_FILENO, "\n", strlen("\n"));
		// }
		if (in_background) {
			print_string("Run in background.\n");
		}

		// Problem 2 - exit, pwd, cd, help cmds
		if (strcmp(tokens[0], "exit") == 0){
			if (tokens[1] != NULL) {
				print_string("Error: exit does not take any arguments\n");
				continue;
			} 
			free(cwd);
			exit(0);
		} 
		else if (strcmp(tokens[0], "pwd") == 0){
			if (tokens[1] != NULL) {
				print_string("Error: pwd does not take any arguments\n");
				continue;
			} 
			print_string(cwd);
			print_string("\n");
			continue;
		} 
		else if (strcmp(tokens[0], "cd") == 0){
			if (chdir(tokens[1]) != 0){
				perror("");
			}
			continue;
		} 
		else if (strcmp(tokens[0], "help") == 0){
			if (tokens[1] == NULL){
				// list all internal commands
				// for each command, include a short summary on what it does
			}
			else if (tokens[2] != NULL) {
				// display an error message
			}
			else if (strcmp(tokens[1], "exit") == 0){
				print_string("'exit' is a builtin command for exiting shell program");
			}
			else if (strcmp(tokens[1], "pwd") == 0){
				print_string("'pwd' is a builtin command for displaying the current working directory");
			}
			else if (strcmp(tokens[1], "cd") == 0){
				print_string("'cd' is a builtin command for changing the current working directory");
			}
			else if (strcmp(tokens[1], "help") == 0){
				print_string("'help' is a builtin command for displaying help information on internal commands");
			}
			else {
				print_string("'");
				print_string(tokens[1]);
				print_string("' is an external command or application");
			}
			continue;
		}

        // Problem 1
        pid_t pid = fork();

        if (pid == -1){
            perror("fork failed");
            exit(1);
        } else if (pid == 0){
			execvp(tokens[0], tokens);
            perror("");
            exit(1);
        } else{
            int status;
            if (!in_background)
                waitpid(pid, &status, 0);

			print_string("Completed Child: ");
            char pid_string[20];
            snprintf(pid_string, sizeof(pid_string), "%d\n", pid);
			print_string(pid_string);
        

        }


		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

	}
	return 0;
}