// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>



#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)

#define HISTORY_DEPTH 10


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
		if (i>0 && buff[i]=='\\' && buff[i+1] == ' ' ){
			for(int j=i; j<num_chars; j++) {
				buff[j] = buff[j+1];
				}
		}
		else{
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
		
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Helper function for printing text
 */
void print_string(char text[])
{
	write(STDOUT_FILENO, text, strlen(text));
}

/**
 * Add command to history
 */
void add_command_to_history(const char *command, char history[HISTORY_DEPTH][COMMAND_LENGTH], int *count)
{
	// exclude !!, !n, !-
	if (command[0] == '!'){
		if (command[1] == '!' || command[1] == '-'){
			return;
		}
		_Bool containsNum = true;
		for (int i = 1; command[i] != '\0'; i++){
			if (!isdigit((unsigned char)command[i])) {
				containsNum = false;
				break;
			}
		}
		if (containsNum && command[1] != '\0'){
			return;
		}
	}
	if (*count >= HISTORY_DEPTH){
		for (int i = 0; i < HISTORY_DEPTH - 1; i++){
			strcpy(history[i], history[i + 1]);
		}
		strncpy(history[HISTORY_DEPTH - 1], command, COMMAND_LENGTH);
		history[HISTORY_DEPTH - 1][COMMAND_LENGTH - 1] = '\0';
	} else {
		strncpy(history[*count], command, COMMAND_LENGTH);
		history[*count][COMMAND_LENGTH - 1] = '\0';
	}
	(*count)++;
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
void read_command(char *buff, char *tokens[], _Bool *in_background, char history[HISTORY_DEPTH][COMMAND_LENGTH], int *count, _Bool need_input)
{
	*in_background = false;

	int length;

	// Read input unless running from history
	if (need_input){
		length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

		if (length < 0) {
			perror("Unable to read command from keyboard. Terminating.\n");
			exit(-1);
		}
	}
	else{
		length = strlen(buff) + 1;
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Store command in History
	add_command_to_history(buff, history, count);

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

void execute_command(char* tokens[], _Bool in_background, int* cmdCount, char history[HISTORY_DEPTH][COMMAND_LENGTH], char cwd[PATH_MAX]){
	if (tokens[0] == NULL){
		return;
	}
	// Problem 2 - exit, pwd, cd, help cmds
	static char prev_dir[PATH_MAX] = "";
	if (strcmp(tokens[0], "exit") == 0){
		if (tokens[1] != NULL) {
			print_string("Error: exit does not take any arguments\n");
			return;
		} 
		exit(0);
	} 
	else if (strcmp(tokens[0], "pwd") == 0){
		if (tokens[1] != NULL) {
			print_string("Error: pwd does not take any arguments\n");
			return;
		} 
		print_string(cwd);
		print_string("\n");
		return;
	} 
	else if (strcmp(tokens[0], "cd") == 0){
	    	int change_dir_result = -1;
	    	char new_dir[PATH_MAX];
	
	    	if (tokens[1] == NULL || strcmp(tokens[1], "") == 0) {
	      		strcpy(new_dir, getenv("HOME"));
	      		change_dir_result = chdir(new_dir);
	   	} else if (strcmp(tokens[1], "~") == 0) {
	      		strcpy(new_dir, getenv("HOME"));
	      		change_dir_result = chdir(new_dir);
	   	} else if (strcmp(tokens[1], "-") == 0) {
	      		if (strcmp(prev_dir, "") != 0) {
				strcpy(new_dir, prev_dir);
				change_dir_result = chdir(new_dir);
	      		} else {
				return;
			}
	   	} else if (tokens[1][0] == '~') {
			if (tokens[2] != NULL) {
				print_string("Error: 'cd ~/{folder_name}' is unable to take more than one parameter.\n");
				return;
			}
	      		snprintf(new_dir, sizeof(new_dir), "%s%s", getenv("HOME"), tokens[1] + 1);
	      		change_dir_result = chdir(new_dir);
	    	} else {
	      		// change to the specified directory
	      		change_dir_result = chdir(tokens[1]);
	    	}
	    	if (change_dir_result == 0) {
	      		// update prev_dir if chdir was successful
	      		strcpy(prev_dir, cwd);
	      		// update the current working directory
	      		getcwd(cwd, PATH_MAX);
	    	} else {
	      		perror("cd");
	    	}
		return;
	} 
	else if (strcmp(tokens[0], "help") == 0){
		if (tokens[1] == NULL){
			print_string("'exit' for exiting the program.\n");
			print_string("'pwd' for displaying the current working directory.\n");
			print_string("'cd' for changing the current working directory.\n");
			print_string("'help' for displaying the help information on internal command.\n");
			print_string("'history' is a builtin command for showing command history\n");
			print_string("'!n' is an internal command for executing the n-th command from history\n");
			print_string("'!!' is an internal command for executing the last command from history\n");
			print_string("'!-' is an internal command for clearing command list history\n");
		}
		else if (tokens[2] != NULL) {
			print_string("Error: help does not take more than one argument\n");
		}
		else if (strcmp(tokens[1], "exit") == 0){
			print_string("'exit' is a builtin command for exiting shell program\n");
		} else if (strcmp(tokens[1], "pwd") == 0){
			print_string("'pwd' is a builtin command for displaying the current working directory\n");
		} else if (strcmp(tokens[1], "cd") == 0){
			print_string("'cd' is a builtin command for changing the current working directory]\n");
		} else if (strcmp(tokens[1], "help") == 0){
			print_string("'help' is a builtin command for displaying help information on internal commands\n");
		} else if (strcmp(tokens[1], "history") == 0){
			print_string("'history' is a builtin command for showing command history\n");
		} else if (strcmp(tokens[1], "!n") == 0){
			print_string("'!n' is an internal command for executing the n-th command from history\n");
		} else if (strcmp(tokens[1], "!!") == 0){
			print_string("'!!' is an internal command for executing the last command from history\n");
		} else if (strcmp(tokens[1], "!-") == 0){
			print_string("'!-' is an internal command for clearing command list history\n");
		}
		else {
			print_string("'");
			print_string(tokens[1]);
			print_string("' is an external command or application\n");
		}
		return;
	}
	else if (strcmp(tokens[0], "history") == 0){
		for (int i = 0; i < (*cmdCount < 10 ? *cmdCount : HISTORY_DEPTH); i++){
			char line[20 + COMMAND_LENGTH];
			char* linetoprocee = history[(*cmdCount < 10 ? *cmdCount : HISTORY_DEPTH) - i - 1];
			// char* character1;
			// char* character2;
			// for(int j=0;j<COMMAND_LENGTH;j++){
			// 	character1= linetoprocee+j;
			// 	character2= linetoprocee+j+1;
			// 	if(*character1=='\0' && *character2!='\0') *character1=' ';
			// }

			sprintf(line, "%d	%s\n", *cmdCount - i - 1, linetoprocee);
			print_string(line);
		}
		return;
	}

	pid_t pid = fork();

	if (pid == -1){
		perror("fork failed");
		exit(1);
	} else if (pid == 0){
		execvp(tokens[0], tokens);
		// execl("/bin/sh", "/bin/sh", "-c", "sleep 2 && echo hi &", (char *) NULL);
		perror("execvp");
		exit(1);
	} else{
		if (!in_background){
			int status;
			waitpid(pid, &status, 0);
		}
		// print_string("Completed Child: ");
		// char pid_string[20];
		// snprintf(pid_string, sizeof(pid_string), "%d\n", pid);
		// print_string(pid_string);
	}
}

void handle_SIGINT(int sig)
{
	print_string("\n'exit' for exiting the program.\n");
	print_string("'pwd' for displaying the current working directory.\n");
	print_string("'cd' for changing the current working directory.\n");
	print_string("'help' for displaying the help information on internal command.\n");
    // exit(0);
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

	char history[HISTORY_DEPTH][COMMAND_LENGTH];
	int cmdCount = 0;

    // Set up the signal handler
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    handler.sa_flags = 0;
    sigemptyset(&handler.sa_mask);
    sigaction(SIGINT, &handler, NULL);


	while (true) {

		// char *cwd = (char*) malloc(sizeof(char) * PATH_MAX);
		char cwd[PATH_MAX];

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
		read_command(input_buffer, tokens, &in_background, history, &cmdCount, true);

		// DEBUG: Dump out arguments:
		// for (int i = 0; tokens[i] != NULL; i++) {
		// 	write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
		// 	write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
		// 	write(STDOUT_FILENO, "\n", strlen("\n"));
		// }
		if (in_background) {
			print_string("Run in background.\n");
		}

		if (tokens[0] == NULL){
			continue;
		}

		if (tokens[0][0] == '!' && tokens[0][1] != '\0') {
			// !!
			if (tokens[0][1] == '!' && tokens[0][2] == '\0'){
				// check if there's previous command
				if (cmdCount > 0){
					int index = cmdCount - 1;
					if (cmdCount > HISTORY_DEPTH - 1)
						index = index - (cmdCount - HISTORY_DEPTH);
					strcpy(input_buffer, history[index]);
					read_command(input_buffer, tokens, &in_background, history, &cmdCount, false);
					execute_command(tokens, in_background, &cmdCount, history, cwd);
				}
				else{
					print_string("No previous command exist in history\n");
				}
				continue;
			}
			// !-
			else if (tokens[0][1] == '-' && tokens[0][2] == '\0'){
				if (cmdCount > 0) {
					for (int i = 0; i < HISTORY_DEPTH; i++) {
						history[i][0] = '\0'; // set first character of each history entry to null
						cmdCount = 0;
					}
				}
				continue;
			}
			// !n
			_Bool containsNum = true;
			for (int i = 1; tokens[0][i] != '\0'; i++){
				if (!isdigit((unsigned char)tokens[0][i])) {
					containsNum = false;
					break;
				}
			}
			if (containsNum){
				// exclude oversize number
				if (atoi(&tokens[0][1]) > cmdCount - 1 || atoi(&tokens[0][1]) < cmdCount - HISTORY_DEPTH){
					print_string("Number does not exist in history\n");
					continue;
				}
				// calculate index
				int index = atoi(&tokens[0][1]);
				if (cmdCount > HISTORY_DEPTH - 1)
					index = atoi(&tokens[0][1]) - (cmdCount - HISTORY_DEPTH);
				print_string(history[index]);
				strcpy(input_buffer, history[index]);
				read_command(input_buffer, tokens, &in_background, history, &cmdCount, false);
				print_string("\n");
				execute_command(tokens, in_background, &cmdCount, history, cwd);
			}
			else {
				// display an error
				print_string("Invalid ! command\n");
			}
			continue;
		}

		// execute cmd
        execute_command(tokens, in_background, &cmdCount, history, cwd);


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
