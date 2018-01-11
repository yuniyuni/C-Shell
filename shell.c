#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h> 
#include <signal.h> 
#include <fcntl.h>  

#define BUFFER_SIZE 1024

#define IN_REDIRECT 		2
#define OUT_REDIRECT 		4
#define OUT_REDIRECT_APPEND	8
#define HAS_PIPE 			16

#define  READ_END 0
#define WRITE_END 1

typedef struct command
{
	int flag;
	char** argv;
	char* in_file_name;
    char* out_file_name;
    struct command* next_cmd; // for pipe
}command;

void type_prompt();

command* analyse_cmd(char cmd_buffer[]);

command get_cmd();

int execute_cmd(command cmd);

void shell();

void signal_handler(int signo);

int main()
{
	if(signal(SIGINT,signal_handler) == SIG_ERR) 
	{
        fprintf(stderr,"signal failed\n");
        return -1;
    }
    shell();
    return 0;
}

void signal_handler(int signo)
{
    shell();
}

void shell() 
{

	while(1)
	{
		type_prompt();

		char cmd_buffer[BUFFER_SIZE];
		fgets(cmd_buffer, BUFFER_SIZE, stdin);

		// ## Clean exit, no memory leak (e.g. Ctrl+d or exit)
		if(feof(stdin) || strcmp(cmd_buffer, "exit\n") == 0) //  command == "^C")
		{
			printf("\n");
			exit(0);
		}	

		// ## Change working directory (e.g. cd /home)
		if(strncmp(cmd_buffer, "cd", 2) == 0)
		{
			char* dir = malloc(strlen(cmd_buffer) - 3 - 1);
			strncpy(dir, cmd_buffer + 3, strlen(cmd_buffer) - 3 - 1);
			if(chdir(dir) == 0)
				continue;
			else
				perror("Wrong directory");
			// TOFIX: ./ ~/ ../ has not been considered yet
		}

		if(strcmp(cmd_buffer,"\n") == 0)
			continue;

		command *cmd = analyse_cmd(cmd_buffer);

		pid_t child_id = fork(); //fork a child
		
		// error check
	    if(child_id < 0) 
	    {
	        perror("fork()");
	        exit(EXIT_FAILURE);
	    }

	    if(child_id == 0)
	    {
	    	// child process
	    	int exit_status = execute_cmd(*cmd);
        	_exit(exit_status);
	    }
	    else 
	    {
	    	// parent process
	    	// wait until children terminate
	   		int status;
	    	waitpid(child_id, &status, 0);

	    	// free cmd
	    	int i;
	    	for(i = 0; i < 32; i++)
			{
				free(cmd->argv[i]);
			}
			free(cmd->argv);
			free(cmd->in_file_name);
			free(cmd->out_file_name);
	    }
	}
}

void type_prompt() 
{
	printf("YuniShell $");
}

command* analyse_cmd(char cmd_buffer[])
{
	command *cmd = malloc(sizeof(command));
	cmd->flag = 0;
	// initiate argv, in_file_name, out_file_name
	char** argv = calloc(32, sizeof(char*));

	int i;
	for(i = 0; i < 32; i++)
		argv[i] = calloc(32, sizeof(char));

	char* in_file_name = calloc(BUFFER_SIZE, sizeof(char));
	char* out_file_name = calloc(BUFFER_SIZE, sizeof(char));

	// seperate argv according to spaces
	i = 0; //end
	int j = 0; //start
	int argv_idx = 0; 
	while(cmd_buffer[i] != '\0'){
		if(cmd_buffer[i] == ' ')
		{
			strncpy(argv[argv_idx++], cmd_buffer + j, i - j);
			j = i + 1;
		}
		
		if(cmd_buffer[i] == '>') // out direct
		{
			if(cmd_buffer[++i] == '>')
				cmd->flag |= OUT_REDIRECT_APPEND;
			else
				cmd->flag |= OUT_REDIRECT;
			break;
		}

		if(cmd_buffer[i] == '<')
		{
			cmd->flag |= IN_REDIRECT;
			break;
		}

		if(cmd_buffer[i] == '|') //pipe
		{
			cmd->flag |= HAS_PIPE;
			break;
		}
		i++;
	}
	
	if(cmd->flag & OUT_REDIRECT || cmd->flag & OUT_REDIRECT_APPEND || cmd->flag & IN_REDIRECT)
	{

		while(cmd_buffer[++i] == ' '); // put i at the start of file name
		j = i;
		
		while(cmd_buffer[++i] != ' ' && cmd_buffer[i] != '\0' && cmd_buffer[i] != '|'); // put i at the end of file name

		if(cmd->flag & IN_REDIRECT){
			if(cmd_buffer[i] == ' ')
				strncpy(in_file_name, cmd_buffer + j, i - j);
			else
				strncpy(in_file_name, cmd_buffer + j, i - j - 1);
		}
		else
			strncpy(out_file_name, cmd_buffer + j, i - j - 1);


		// ## Wait for command to be completed when encountering >
		while(cmd->flag & OUT_REDIRECT && strlen(out_file_name) == 0)
		{
			type_prompt();
			printf(">");
			fgets(out_file_name, BUFFER_SIZE, stdin);
			out_file_name[strlen(out_file_name)-1] = '\0';  //handle \n
		}	

		if(cmd_buffer[i] == ' ')
		{
			while(cmd_buffer[++i] != '\0' && cmd_buffer[i] != '|');
			if(cmd_buffer[i] == '|')
				cmd->flag |= HAS_PIPE;
		}

		// TOFIX: having both > < is not considered yet
	}


	if(cmd->flag & HAS_PIPE){
		// lead i&j to the start of the 2nd argv.
		
		while(cmd_buffer[++i] == ' ');
		j = i; 

		char cmd_buffer2[BUFFER_SIZE];
		strncpy(cmd_buffer2, cmd_buffer + i, strlen(cmd_buffer) - i + 1);  // +1 to append \0

		// ## Wait for command to be completed when encountering |
		while(strlen(cmd_buffer2) == 0 || cmd_buffer2[0] == '\n')
		{
			type_prompt();
			printf("|");
			fgets(cmd_buffer2, BUFFER_SIZE, stdin);
		}

		command* next_cmd = analyse_cmd(cmd_buffer2);
		cmd->next_cmd = next_cmd;
	}
	else
	{
		cmd->next_cmd = NULL;
	}

	if(cmd->flag == 0){
		strncpy(argv[argv_idx++], cmd_buffer + j, i - j - 1);
	}
	argv[argv_idx] = (char*) NULL;

	cmd->argv = argv;
	cmd->in_file_name = in_file_name;
	cmd->out_file_name = out_file_name;
    return cmd;
}




int execute_cmd(command cmd)
{
	if(strcmp(cmd.argv[0], "echo") == 0)
	{
		// remove quotes, actually dont understand the instruction...
		char **p = cmd.argv;
    
	    while(p != NULL && *p != NULL)
	    {
	        int i;
	        int size = sizeof(*p);
	        char *temp = calloc(sizeof(char), size);
	        int offset = 0;
	        for(i = 0; i < size; i++)
	        {
	            if(*(*p+i) == '"')
	                offset++;
	            else
	                temp[i-offset] = *(*p+i);
	        }
	        
	        *p = temp;
	        
	        p++;
	    }

	}

	if(cmd.flag & HAS_PIPE)
	{
		// Holds file descriptors for read and write ends of the pipe
	    int pipefd[2];

	    // Create the (unidirectional) pipe
	    if (pipe(pipefd)) 
	    {
	        perror("pipe");
	        exit(EXIT_FAILURE);
	    }
		
	    pid_t command_1_pid = fork();
	    if(command_1_pid < 0) 
	    {
	    	perror("fork command 1 child");
	    	return EXIT_FAILURE;
	    }
	    else if(command_1_pid == 0)
	    {
	    	// child
	    	// redirect stdout to pipe write
	    	close(pipefd[READ_END]);
	    	if(close(STDOUT_FILENO) == -1)
	    		perror("close STDOUT_FILENO");
		    dup2(pipefd[WRITE_END], STDOUT_FILENO);

		    if(cmd.flag & IN_REDIRECT)
			{
				// redirect stdin 
				int in_fd = open(cmd.in_file_name, O_RDONLY, 0666);
		        close(STDIN_FILENO); 
		        dup2(in_fd, STDIN_FILENO);
		        close(in_fd); 		     	        
			}
			
			fflush(stdin);
			fflush(stdout);
			execvp(cmd.argv[0], cmd.argv);

		    _exit(EXIT_FAILURE);
	    }

	    pid_t command_2_pid = fork();
	    if(command_2_pid < 0)
	    {
	    	perror("fork command 2 child");
	    	return EXIT_FAILURE;
	    }
	    else if(command_2_pid == 0)
	    {
	    	close(pipefd[WRITE_END]);
	
    		close(STDIN_FILENO);
    		dup2(pipefd[READ_END], STDIN_FILENO);
	       	
	       	fflush(stdin);
			fflush(stdout);
	    	execute_cmd(*cmd.next_cmd); 
	    	_exit(EXIT_FAILURE);
	    }

	    close(pipefd[READ_END]);
    	close(pipefd[WRITE_END]);

	    // Wait for children
	    int status1, status2;
	    wait(&status1);
	    wait(&status2);

	    // If either status = 1 (FAILURE)
	    // then the overall process failed
	    return (status1 | status2);
        
	}

	if(cmd.flag & OUT_REDIRECT)
	{
		int fd = open(cmd.out_file_name,O_WRONLY|O_CREAT|O_TRUNC,0666);  	
        dup2(fd,STDOUT_FILENO);  
        execvp(cmd.argv[0], cmd.argv);  
	}
	else if(cmd.flag & OUT_REDIRECT_APPEND)
	{
		int fd = open(cmd.out_file_name,O_WRONLY|O_CREAT|O_APPEND,0666);  	
        dup2(fd,STDOUT_FILENO);  
        execvp(cmd.argv[0], cmd.argv);  
	}

	if(cmd.flag & IN_REDIRECT)
	{		
		int in_fd = open(cmd.in_file_name, O_RDONLY, 0666);
        close(STDIN_FILENO); 
        dup2(in_fd, STDIN_FILENO);
        execvp(cmd.argv[0], cmd.argv);  
        close(in_fd); 
	}

	if(cmd.flag == 0)
		execvp(cmd.argv[0], cmd.argv);
		
	perror("Error calling exec()");
    return EXIT_FAILURE;
}


