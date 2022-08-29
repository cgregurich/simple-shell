
/*
 * Auth: Colin Gregurich
 * Date: 8-6-22 (Due: 8-12-22)
 * Course: CSCI-4500 (Sec: 850)
 * Proj#: 1 (Simple Shell)
 * Desc: A simple shell program using the pieces built through the pre-projects.
 * It emulates a bash shell and makes use of various system calls to allow the
 * user to enter jobs to execute.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_CMDLINE_SIZE 256 
#define MAX_JOBS_PER_LINE 10
#define MAX_SEQOPS 10
#define MAX_SIMPLE_CMDS 10
#define MAX_ARGS_PER_CMD 10

void show_prompt(char* prompt);
int is_cmdline_empty(char* cmdline);
int readline(int fd, char *buf, int bufsz);
int writeline(int fd, const char *str);
int split_into_jobs(char* cmdline, char* jobs[], size_t count);
int scan_seqops(int seqops[], char* jobstr, size_t count);
int extract_simple_cmds(char* jobstr, char* simple_cmds[], size_t count);
int extract_cmd_args(char* simple_cmd, char** cmd, char* cmdargs[], size_t count);
int command_exists(char* command, char* full_path_buffer);
int command_contains_slash(char* command);
int execute_simple_command(char* full_command_path, char* cmdargs[], int cmd_arg_count, int seqops[], int index);
void execute_command(char* cmd_path, char* cmdargs[], int arg_count);

int main(void) {
	while(1) {
		show_prompt(">> ");
		char cmdline[MAX_CMDLINE_SIZE];

		/* Read input from user */
		int bytes_read = readline(0, cmdline, MAX_CMDLINE_SIZE);
		
		/* If user entered no characters or only whitespace, go back to showing
		prompt and waiting for input */
		if (bytes_read == 1 || is_cmdline_empty(cmdline)) {
			continue;
		}

		/* If user sends EOF exit shell */
		if (bytes_read <= 0) {
			writeline(1, "\n");
			exit(EXIT_SUCCESS);
		}

		/* Parse jobs from user input */
		char* jobs[MAX_JOBS_PER_LINE];
		int job_count = split_into_jobs(cmdline, jobs, MAX_JOBS_PER_LINE);

		/* For each job, parse seqops and simple commands from it */
		for (int job_index=0; job_index<job_count; job_index++) {

			/* Parse seqops from job */
			int seqops[MAX_SEQOPS];
			scan_seqops(seqops, jobs[job_index], MAX_SIMPLE_CMDS);

			/* Parse simple commands from job */
			char* simple_cmds[MAX_SIMPLE_CMDS];
			int simple_cmd_count = extract_simple_cmds(jobs[job_index], simple_cmds, MAX_SIMPLE_CMDS);

			/* For each simple command, parse command and args, check if command exists, and execute it if it does */	
			for (int simple_cmd_index=0; simple_cmd_index<simple_cmd_count; simple_cmd_index++) {

				/* Parse cmd and args from simple command */
				char* cmdargs[MAX_ARGS_PER_CMD];
				char* cmd;
				int arg_count = extract_cmd_args(simple_cmds[simple_cmd_index], &cmd, cmdargs, MAX_ARGS_PER_CMD);


				/* Check if command can be found */
				char full_command_path[256];
				
				/* If it can, execute it */
				if (command_exists(cmd, full_command_path)) {

					/* Executing command returns value indicating if the next command should be executed */
					int exec_next_cmd = execute_simple_command(full_command_path, cmdargs, arg_count, seqops, simple_cmd_index);
					/* If not, break from the simple_commands loop to go to the next simple command */
					if (!exec_next_cmd) {
						break;
					}
				}
				/* If command couldn't be found, tell the user */
				else {
					writeline(1, "ERROR: file or command not found!\n");
				}
			}
		}
	}
	exit(EXIT_SUCCESS);
}

void show_prompt(char* prompt) {
/* -------------------------------------------------------- */
/* Desc:
    Function show_prompt() shows the passed prompt, and flushes stdout.

   Input Args:
    char* prompt: String prompt to be displayed.

   Returns:
    n/a
*/
	writeline(1, prompt);
	fflush(stdout);
}

int is_cmdline_empty(char* cmdline) {
/* -------------------------------------------------------- */
/* Desc:
    Function is_cmdline_empty() checks if the given cmdline contains only spaces
	and/or tabs. If it does, returns 1. Otherwise, returns 0.

   Input Args:
    char* cmdline: the cmdline to be checked

   Returns:
    1 if cmdline only contains spaces and/or tabs, otherwise 0.
*/
	int i = 0;
	while (cmdline[i] != '\0') {
		if (cmdline[i] != ' ' && cmdline[i] != '\t') {
			return 0;
		}
		i++;
	}
	return 1;
}

int readline(int fd, char* buf, int bufsz) {
/* -------------------------------------------------------- */
/* Desc:
    Function readline() reads a line from the given file descriptor and stores
	the read data in the given buffer.

   Input Args:
    int fd: file descriptor to read from.
	char* buf: buffer to store the data read.
	int bufsz: size of the buffer.

   Returns:
	 int: Number of characters read.
    
*/
    int charCount = 0;
    int readResult;
    int bufIndex = 0;
    while (1) {
        readResult = read(fd, buf + bufIndex, 1);
        if (readResult == -1) {
            /* Error occurred */
            return -1;
        }
        else if (readResult == 0) {
            /* EOF reached */
            return 0;
        }
        else {
            charCount += readResult;
        }
        if (buf[bufIndex] == '\n') {
            buf[bufIndex] = '\0';
            break;
        }
        bufIndex++;
    }
    return charCount;
}

int writeline(int fd, const char *str) {
/* -------------------------------------------------------- */
/* Desc:
    Function writeline() writes data to the given file descriptor.

   Input Args:
    int fd: file descriptor to write data to.
	char* str: String containing data to write.

   Returns:
    int: Returns number of characters written.
    
*/
    /* Maximum number of loop iterations */
    const size_t MAXSTRLEN = 256;
    int charCount = 0;
    int i = 0;
    char nextChar;
    int writeResult;
    while (1) {
        if (i > MAXSTRLEN) {
            break;
        }
        if (*(str + i) == '\0') {
			// Had to modify this function to not automatically include \n so that
			// it could be used for showing the shell prompt
			break;
        }
        else {
            nextChar = *(str + i);
        }
        writeResult = write(fd, &nextChar, 1);

        /* If an error occurred with write() system call */
        if (writeResult == -1) {
            return -1;
        }
        else {
            charCount += writeResult;
        }
        if (nextChar == '\n') {
            break;
        }
        i++;
    }
    return charCount;
}

int split_into_jobs(char* cmdline, char* jobs[], size_t count) {
/* -------------------------------------------------------- */
/* Desc:
    Function split_into_jobs() splits string cmdline by delimiter ";" to find 
	each job in the line entered by user, and return the number of jobs.

   Input Args:
    char* cmdline: The line entered by user.
	char* jobs[]: Array to store the jobs found in the line.
	size_t count: The max number of jobs to be found in the array.

   Returns:
    int: Number of jobs found in the process
*/
	int jobs_index = 0;
	char* piece = strtok(cmdline, ";");
	while (piece != NULL) {
		jobs[jobs_index++] = piece;
		piece = strtok(NULL, ";");
	}
	return jobs_index;
}

int scan_seqops(int seqops[], char* jobstr, size_t count) {
/* -------------------------------------------------------- */
/* Desc:
    Function scan_seqops() goes through string of a given job and each time "&&"
	or "||" occurs, adds either a 1 or 2 to array seqops, respectively. Returns
	the number of times one of these sequence operators was found.

   Input Args:
	int seqops[]: Array to store the sequence operators' values.
	char* jobstr: The job to be parsed.
	size_t count: The max number of sequence operators.

   Returns:
	int: The number of sequence operators found.
*/
	int i = 0;
	int seqops_index = 0;
	while (jobstr[i++] != '\0') {
		if (jobstr[i] == '&' && jobstr[i+1] == '&') {
			seqops[seqops_index++] = 1;
		}
		else if (jobstr[i] == '|' && jobstr[i+1] == '|') {
			seqops[seqops_index++] = 2;
		}
	}
	return seqops_index;
}

int extract_simple_cmds(char* jobstr, char* simple_cmds[], size_t count) {
/* -------------------------------------------------------- */
/* Desc:
    Function extract_simple_cmds() splits string jobstr by delimiter "&" or "|"
	to find each simple command in the job. Returns the number of simple commands
	found in this process.

   Input Args:
	char* jobstr: String of the job to be parsed.
	char* simple_cmds[]: Array to store the simple commands found.
	size_t count: Max number of simple commands.

   Returns:
    int: Number of simple commands found.
*/
	char* piece = strtok(jobstr, "&|");
	int i = 0;
	while (piece != NULL) {
		simple_cmds[i++] = piece;
		piece = strtok(NULL, "&|");
	}
	return i;
}

int extract_cmd_args(char* simple_cmd, char** cmd, char* cmdargs[], size_t count) {
/* -------------------------------------------------------- */
/* Desc:
	Function extract_cmd_args() splits string simple_cmd by delimiter " " (space)
	to find each piece of the simple command. The first piece of a simple command
	is the command itself, stored in cmd. The rest of the pieces are arguments 
	for the command, stored in array cmdargs. 
	Be careful because using strtok here modifies the string simple_cmd by reference.

   Input Args:
	char* simple_cmd: String containing the simple command to be parsed.
	char** cmd: The location to store the command parsed from simple_cmd.
	char* cmdargs[]: Array to store the args parsed from simple_cmd.
	size_t count: Max number of command arguments.

   Returns:
    int: Number of command arguments found.
*/
	int i = 0;
	char* piece = strtok(simple_cmd, " ");
	*cmd = piece;
	while ((piece = strtok(NULL, " ")) != NULL) {
		cmdargs[i++] = piece;
	}
	return i;
}

int command_exists(char* command, char* full_path_buffer) {
/* -------------------------------------------------------- */
/* Desc:
	Function command_exists checks if the given command exists in the PATH.
	If it does, it stores the full path of that command in full_path_buffer
	and returns 1. If it couldn't be found, it returns 0. This handles commands
	such as "ls", "/usr/bin/ls", and "./ls", etc. It checks if there is a slash
	in the command, and if there is, it won't search the PATH for the command.

   Input Args:
	char* command: The command to be located.
	char* full_path_buffer: Location to store the full command path.

   Returns:
    int: 1 if command was located, otherwise 0.
*/
	char* DELIMITER = ":";
	char* path = getenv("PATH");
	char pathcopy[strlen(path)];
	strcpy(pathcopy, path);
	char* piece = strtok(pathcopy, DELIMITER);
	while (piece != NULL) {
		if (command_contains_slash(command)) {
			/* Assume command is absolute path */
			sprintf(full_path_buffer, "%s", command);
		}
		else {
			/* Assume command is not absolute path */
			sprintf(full_path_buffer, "%s/%s", piece, command);
		}
		if (access(full_path_buffer, X_OK) == 0) {
			return 1;
	 	}
		piece = strtok(NULL, DELIMITER);
	}
	return 0;
}

int command_contains_slash(char* command) {
/* -------------------------------------------------------- */
/* Desc:
	Function command_contains_slash() checks if the given command has a slash
	anywhere. This is used to check if a command was given as an absolute path.

   Input Args:
	char* command: The command to be checked.

   Returns:
    int: 1 if the command contains a slash, otherwise 0.
*/
	int i = 0;
	while (command[i] != '\0') {
		if (command[i] == '/') {
			return 1;
		}
		i++;
	}
	return 0;
}


int execute_simple_command(char* full_command_path, char* cmdargs[], int cmd_arg_count, int seqops[], int index) {
/* -------------------------------------------------------- */
/* Desc:
	Function execute_simple_command() takes care of executing a command such as
	"ls -l -a && cal || echo hello". This is to keep main a little cleaner.
	It forks the process then executes the command in the child process.
	The parent process waits for the child to complete. When it does, it uses 
	the status in conjunction with the next sequence operator to decide
	if the next command should be executed. If it should be, 1 is returned. If
	not, 0 is returned.

	It's important to note that this function doesn't take care of the case
	where the entered command doesn't exist; this function is only called if
	the command exists.

	To decide if the next command should be run:
	Mappings used:
	Command succeeded -> 0
	Command failed -> non-zero
	Seqop && -> 1
	Seqop || -> 2

	success and && -> execute next command
	failure and || -> execute next command

	The wait status of a command that succeeded is 0.
	The wait status of a command that failed is 256 aka non-zero.
	If the command succeeded and the next seqop is &&, then we should execute
	the next command.
	If the command failed and the next seqop is ||, then we should execute the
	next command.
	In any other situation, such as command success and next seqop is ||, or 
	command failure and next seqop is &&, then the next command should not be
	executed.

   Input Args:
	char* full_command_path: The absolute path to the command the user entered.
	char* cmdargs[]: The command args the user entered.
	int cmd_arg_count: The number of command args the user entered.
	int seqops[]: The sequence operators for the current job.
	int index: The index of the current simple command. This is used to look at
		the correct sequence operator.

   Returns:
    int: 1 if the next command should be executed, otherwise 0.
*/
	int pid = fork();
	if (pid == 0) { // if child process
		execute_command(full_command_path, cmdargs, cmd_arg_count);
	}
	else { // if parent process
		int status;
		wait(&status);
		if (status == 0 && seqops[index] == 1) {
			return 1;
		}
		else if (status != 0 && seqops[index] == 2) {
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;
}

void execute_command(char* cmd_path, char* cmdargs[], int arg_count) {
/* -------------------------------------------------------- */
/* Desc:
	Function execute_command() executes a single command (as opposed to a simple
	command). This function is responsible for packaging up the given command 
	and command arguments in a way that can be used by execve().
	execve() has three args: cmd, argv, and envp.
	cmd is just the command. 
	argv is an array of char* where the 0th element needs to be the command,
	and the last element needs to be NULL, and every element in between needs
	to be the command args.
	envp also needs to have NULL as the last element.

   Input Args:
	char* cmd_path: The absolute path to the command the user entered.
	char* cmdargs[]: The command args the user entered.
	int cmd_arg_count: The number of command args the user entered.

   Returns:
    n/a
*/
	/* argv needs cmd as element 0 and NULL as the last element, with the args
	in between, therefore size arg_count + 2 */
	char* argv[arg_count + 2];
	argv[0] = cmd_path;
	for (int i=0; i<arg_count; i++) {
		argv[i+1] = cmdargs[i];
	}
	argv[arg_count + 1] = NULL;
	char* envp[] = {NULL};
	execve(cmd_path, argv, envp);
}
