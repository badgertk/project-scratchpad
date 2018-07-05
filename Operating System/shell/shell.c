#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXJOBS	16


pid_t process_id; // process ID
// a line of program invocations divided by pipes will be split into individual programs + their arguments
char** line_tokens;
// a program + their arguments will be split into their individual arguments
char** program_tokens;

char* current_line_token;
char* current_program_token;
char* input_copy;
// need a jobs organization structure
// job listing data structure, taken from sh-skeleton.c
struct job_t{
	pid_t process_id;
	int job_id;
	int state;
	char cmd_line[MAXJOBS];
};
struct job_t job_list[MAXJOBS];
int job_count;

void clear_job_table(struct job_t* job);
void initialize_job_table(struct job_t* job);
int add_job(struct job_t *jobs, pid_t process_id, int state, char *cmd_line);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t get_foreground_pid(struct job_t* jobs);
struct job_t *get_job_pid(struct job_t *jobs, pid_t pid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);



int line_token_index = 0;
int program_token_index = 0;
int loop_increment;
int children = 0;
int exit_status;
int uses_input;
int uses_output;
int pipe_read;
int pipe_write;
int background; // is this a process that starts in the background? parse last arg to be '&'
int invalid_program;


void quit();
void execute(char* process);
void printout_jobs(struct job_t *job);
void wait_children();
void stop_child(pid_t process_id);

//internal commands: do not fork, parent process handles it
// cd, quit, jobs

/**
Just a screen printout to show that shell is running.  Also to indicate when waiting for user input.
**/
void cmd_prompt(){
	printf("WuShell> ");
	fflush(stdout);
}

void child_prompt(int pid){
	printf("CHILD %d:", pid);
}

void parent_prompt(int pid){
	printf("PARENT:");
}

int parse(char* line){
	line_tokens = malloc (64 * sizeof(char*));
	//char* current_line_token;
	line_token_index = 0;
	
	
	input_copy = line;
	// All lines should be in the following structure 
	// <program_name> <parameter 1> ... <parameter n>  | <program_name> <parameter 1> ... <parameter n>
	// <output of program> | <input of program>
	// NOTE: some programs don't have any paramters
	

	//char* last_token = line[strlen(line)];
	
	//strcpy(program_tokens[program_token_index], last_token);
	// end of this little algorithm
	
	while (current_line_token = strtok_r(input_copy, "|", &input_copy)){
		line_tokens[line_token_index] = current_line_token;
		
		//printf("Program %d: |%s|\n", line_token_index, line_tokens[line_token_index]);
		
		//execute(line_tokens[line_token_index]);
		
		line_token_index++;
	}
	//printf("How many program_tokens? %d\n", line_token_index);
	int n = 0; 
	for (n = 0; n<= (line_token_index - 1); n++){
		//printf("execute %d times\n", n);
		pipe_read = (n!= 0) ? 1 : 0;
		pipe_write = (n != line_token_index - 1) ? 1 : 0;
		execute(line_tokens[n]);
	}
	
	
	// cleaning up after yourself
	/****/
	memset(line_tokens, 0 , sizeof(line_tokens));
	free (line_tokens);
	//printf("freed line_tokens\n");
	return 0;
}

void execute(char* process){
	
	//printf("The execute subroutine can read process %d: %s\n", line_token_index, process/**line_tokens[line_token_index]**/);
	// parse the spaces
	// cd <path>
	// quit (no other args)
	// jobs 
	
	/**
	Handle internal commands
	**/
	program_tokens = malloc (64 * sizeof(char*));
	//program_args = malloc (64 * sizeof(char*));
	program_token_index = 0;
	while (current_program_token = strtok_r(process, " \t", &process)){ //WORKING accomodates spaces and tab characters
/**		if ((strcmp(current_program_token, "<") == 0) || (strcmp(current_program_token, ">") == 0)){
			strcat(program_tokens[program_token_index], program_tokens[program_token_index +1]);
			strcpy(program_tokens[program_token_index+1+n], program_tokens[program_token_index+2+n]);
		}**/
		if (current_program_token[0] == '<'){
			uses_input = 1;
			printf("USES INPUT!\n");
		}
		if (current_program_token[0] == '>'){
			uses_output = 1;
			printf("USES OUTPUT!\n");
		}
		program_tokens[program_token_index] = current_program_token;
		printf("[%s]", program_tokens[program_token_index]);		
		program_token_index++;
	}
	
	int i = 0;
	for (i = 0; i < program_token_index; i++){
	//	strcpy(program_args[i],program_tokens[i+1]);
	}
	
	printf("\n");
	// handle the internal commands exit, cd, and jobs
	if (strcmp(program_tokens[0],"exit") == 0){ //WORKING
		printf("User requested an exit\n");
		process_id = 0;
		quit();
	}
	else if (strcmp(program_tokens[0],"cd") == 0){
		printf("The new path is .%s.", program_tokens[1]);
		chdir(program_tokens[1]);
	}
	else if (strcmp(program_tokens[0],"jobs") == 0){
		printf("User wants a jobs printout\n");
		// Handle this later
		//printout_jobs(struct job_t *job);
	}
	// this isn't one of the internal commands, time to fork a child to do it.
	else {
		//printf("GOT DOWN HERE!\n");

	
	//return pipe_setup[0];
	
	if (strcmp(program_tokens[program_token_index - 1], "&") == 0){
		background = 1;
		printf("User has requested this process be started in the background\n");
	}
	if (process_id < 0){
		//close(pipe_setup[1]);
		printf("fork did not work");
	}
	else{
		int pipe_setup[2];
		pipe(pipe_setup);
		if ((process_id = fork()) == 0){
			//child_prompt(process_id);
			
			// pipe_setup[0] is the input side
			// pipe_setup[1] is the output side
			
			if ((process_id = fork()) > 0){
			//dup2(pipe_setup[1], STDOUT_FILENO);
			dup2(STDOUT_FILENO, pipe_setup[1]);
			//dup2(pipe_setup[0], STDIN_FILENO);
			dup2(STDIN_FILENO, pipe_setup[0]);
			//child_prompt(process_id);
			//printf("duplicated the pipes");
			if (!pipe_read){
				close(pipe_setup[0]);
			}
			if (!pipe_write){
				close(pipe_setup[1]);
			}
			//close(pipe_setup[1]);
			//close(pipe_setup[0]);
			//child_prompt(process_id);
			//printf("closed some of the pipes\n");
			execvp(program_tokens[0], program_tokens);
			// I couldn't execute your program.
			fprintf(stderr, "ERROR: I couldn't execute your program!");
			//exit(0);
			
			if (pipe_read){
				close(pipe_setup[0]);
			}
			if (pipe_write){
				close(pipe_setup[1]);
			}
			
			
			}
		}
		else {//if ((process_id = fork()) > 0){
			sleep(1);
			parent_prompt(process_id);

			if (!pipe_read){
				close(pipe_setup[0]);
			}
			if (!pipe_write){
				close(pipe_setup[1]);
			}
			children++;
			printf("I have %d children\n", children);
			printf("waiting for child with PID %d\n", process_id);
/* 			while(invalid_program){
				stop_child(process_id);
				break;
			} */
			kill (process_id, SIGCHLD);
			waitpid(process_id, &exit_status,0);
			printf("child with PID %d exited with status %d\n", process_id, exit_status);
			if (pipe_read){
				close(pipe_setup[0]);
			}
			if (pipe_write){
				close(pipe_setup[1]);
			}
			children--;
			fflush(stdin);
			fflush(stdout);
			//wait_children();
		}


		
	}
	
	
	}
	// cleaning up after yourself
	memset(program_tokens, 0, sizeof(program_tokens));
	free (program_tokens);
	
	///return;
}

int cmd_loop(){
	char line[1024];
	memset(line, 0, sizeof(line));
	line[0] = '\0';
	cmd_prompt();
	fgets(line,1024,stdin);//read line
	//cmd_prompt();
	// trim off new line at the end
	line[strlen(line) - 1] = '\0';
	
	//printf("I've read: .%s.", line);
	if (strcmp(line,"\n") == 0){//ignore if just a carriage return
		return 0;
	}
	
	parse(line); // parse line
	return 0;
}

/**
The shell's main routine.
**/
int main (int argc, char *argv[]){
	
	// Check for proper launch of shell
	if (argc <= 3){
		fprintf(stderr, "ERROR: Improper arguments!\nUSAGE: shell\n       shell <input.file\n       shell <input.file >output.file\n", argv[0]);
		return -1;
	}
	
	if (argc == 2 || argc == 3){
		if (argv[1][0] == '<'){
			memmove(argv[1], argv[1] + 1 , sizeof(argv[1]);
		
		FILE* input_file;
		
		input_file = fopen(argv[2], "r");
		read(input_file, line, sizeof(input_file);
		fclose(input_file);
		}
		if (argv[1][0] == '>' ){
			memmove(argv[1], argv[1] + 1 , sizeof(argv[1]);
		
		FILE* output_file;
		
		output_file = fopen(argv[2], "w");
		//read(output_file, line, sizeof(output_file);
		fclose(output_file);
		}
		if (argv[2][0] == '>' ){
			memmove(argv[3], argv[3] + 1 , sizeof(argv[1]);
		
		FILE* output_file;
		
		output_file = fopen(argv[3], "w");
		//write(output_file, line, sizeof(output_file);
		fclose(output_file);
		}
	}
	
	// Load config files
	
	// Run command loop
	while (1){
	
	cmd_loop(/**Need some sort of return signal here to break properly from this I think.**/);
	}
	
	// Perform shutdown and cleanup
	
	quit();
	return 0;
}


void clear_job_table(struct job_t* job){
	job->process_id = 0;
	job->job_id = 0;
	job->state = 0;
	job->cmd_line[0] = '/0';
}
void initialize_job_table(struct job_t* job){
	int i;
	
	for (i = 0; i < MAXJOBS; i++){
		clearjob(&jobs[i]);
	}
}
int add_job(struct job_t *jobs, pid_t process_id, int state, char *cmd_line){
	int i;
	
	if (pid < 1){
		return 0;
	}
	for (i = 0; i < MAXJOBS; i++){
		
	}
	
}
int deletejob(struct job_t *jobs, pid_t pid);
pid_t get_foreground_pid(struct job_t* jobs){
	int i;
	
	for (i = 0; i < MAXJOBS; i++){
		if (jobs[i].state == foreground){
			return jobs[i].process_id;
		}
		return 0;
	}
}

struct job_t *get_job_pid(struct job_t *jobs, pid_t pid){
	int i;
	
	if (pid < 1){
		return NULL;
	}
	for (i = 0; i < MAXJOBS; i++){
		return &jobs[i];
	}
	return NULL
}

int pid2jid(pid_t pid){
	int i;
	
	if (process_id < 1){
		return 0;
	}
	for (i = 0; i < MAXJOBS; i++){
		if (jobs.[i]).process_id == process_id){
			return jobs[i].job_id;
		}
	}
	return 0;
}

void list_jobs(struct job_t *jobs){
	int i;
	
	for (i = 0; i < MAXJOBS; i++){
		if(jobs[i].process_id != 0){
			printf("[%d] (%d) ", jobs[i].job_id, jobs[i].process_id);
			case background:
				printf("Running ");
				break;
			case foreground:
				printf("Foreground ");
				break;
			case stopped:
				printf("Stopped ");
				break;
			default:
				printf("Job state invalid! job[%d].state=%d ", i, jobs[i].state);
		}
		printf("%s\n", jobs[i].cmd_line);
	}
}

/**
void printout_jobs(struct job_t *job){
	// index number for the command line that was run in the background
	// the command line that was planed into the background
	char* process_status[] = {"Foreground", "Background", "Running", "Stopped"};
	for (loop_increment = 0; loop_increment < job_count; loop_increment++){
		printf("[%d] (%d) ", job_list[loop_increment].jid, job_list[loop_increment].pid);
	    switch (job_list[loop_increment].process_status) {
		case Background: 
		    printf("Running ");
		    break;
		case Foreground: 
		    printf("Foreground ");
		    break;
		case Stopped: 
		    printf("Stopped ");
		    break;
	    default:
			break;
	    }
	    printf("%s", job_list[loop_increment].cmdline);
		if (Background){
			printf("\t[&]\n");
		}
	}
	
	
	
	printf("<id> : <program_name> <arg>\n");
	printf("==================================\n");
	for (loop_increment = 0; loop_increment < children; loop_increment++){
		printf("%d : %s\n", children, program_tokens);
	}
	
	// if no internal commands
	printf("<program_name> <arg> [&]\n");
	printf("==================================\n");
	
	// current running program in foreground
	// background need & suffix
}**/

void wait_child(){
	int i;
	printf("waiting for children");
	for (i = 1; i < children; i++){

	}
}

void quit(){
	// quit parent process
	kill (process_id, SIGQUIT);

}

void interrupt(){
	// Ctrl+C signal interrupt 
	kill (process_id, SIGINT);
}

void stop(){
	// Ctrl+Z signal stop 
	kill (process_id, SIGTSTP);
}

void stop_child(pid_t process_id){
	// Terminated or stopped child process 
	kill (process_id, SIGCHLD);
}
