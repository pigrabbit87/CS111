// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "alloc.h"
int num_stream = 0;
enum I_O
{
	INPUT,
	OUTPUT,
};

struct io_struct
{
	enum I_O io_type;
	char* file_name; 
	io_struct_t next;
};

int
command_status (command_t c)
{
  return c->status;
}

int 
redirect (command_t c)
{
	if (c->input != 0)
	{
		char *fn = c->input; //file name
		FILE *fp = fopen(fn, "r"); 
		if (fp == NULL)
		{
			if (errno == ENOENT)
				fprintf(stderr, "File not available\n");
			else
				fprintf(stderr, "Error occur reading the file \n");
			return 1;
		}
		dup2(fileno(fp), 0);
		fclose(fp);
	}
	if (c->output != 0)
	{
		char *fn = c->output;
		FILE *fp = fopen(fn, "w"); //TO-DO error checking
		if (fp == NULL)
		{
			if (errno == ENOENT)
				fprintf(stderr, "File not available\n");
			else
				fprintf(stderr, "Error occur opening the file \n");
			return 1;
		}
		dup2(fileno(fp), 1);
		fclose(fp);
	}
	return 0;
}

int exec(char* c)
{
	if (c[0] != 'e') return 0;
	if (c[1] != 'x') return 0;
	if (c[2] != 'e') return 0;
	if (c[3] != 'c') return 0;
	if (c[4] != '\0') return 0;
	return 1;
}

int 
func (command_t c) //exception for exec
{
	if (redirect(c) == 1) return 1; // changes input and output carrot
	
	int err;
	
	switch(c->type)
	{
		case SIMPLE_COMMAND:
		{
			pid_t first_child = fork();
			if (first_child < 0)
			{
        			fprintf(stderr, "Cannot create child process \n");
        			exit(1);
			}
			else if (first_child == 0) // child process
			{
				int status;
				if (exec(c->u.word[0]))
					status = execvp(c->u.word[1], c->u.word+1);
				else
					status = execvp(c->u.word[0], c->u.word);
				c->status = status;
				fprintf(stderr, "Unknown command \n");
				exit(1);
			}
			else 
			{
				int status = 0;
				waitpid(first_child, &status, 0);
				if(status != 0)
					return 1;
				else
					return 0;
			}
			break;
		}
		case AND_COMMAND:
		{
			pid_t pid = fork();
			if (pid < 0) {
        		fprintf(stderr, "Cannot create child process \n");
        		exit(1);
			} 
			else if (pid == 0) 
			{
                		if(func(c->u.command[0]) == 0) //successful
				{
					_exit(0);
				}
				else
				{
					_exit(1);
				}
            	} 
			else{		
				int status = 0;
                		waitpid(pid, &status, 0);
				if (status == 0)
				{
					func(c->u.command[1]);
				}
            	}
			break;
		}
		case OR_COMMAND:
		{
			pid_t pid = fork();
			if (pid < 0) 
			{
                		fprintf(stderr, "Cannot create child process \n");
                		exit(1);
            	} 
			else if (pid == 0) 
			{
                		if(func(c->u.command[0]) == 0) //successful
				{
					_exit(0);
				}
				else
				{
					_exit(1);
				}
            	} 
			else{		
				int status = 0;
                		waitpid(pid, &status, 0);
				if (status != 0)
				{
					func(c->u.command[1]);
				}
			}
            	break;
		}
		case SUBSHELL_COMMAND:
		{
			func(c->u.subshell_command);
			_exit(0);
			break;
		}
		case PIPE_COMMAND:
		{
			int pipefd[2];
			pid_t child = fork();
			if (child < 0) 
			{
                		fprintf(stderr, "Cannot create child process \n");
                		exit(1);
			}
			else if (child == 0) //child process
			{
				int c_child;
				err = pipe(pipefd);
				if(err == -1) perror("pipe error \n");

				c_child = fork();
				if(c_child < 0)
				{
					fprintf(stderr, "Cannot create child process \n");
                			exit(1);
				}
				else if(c_child == 0)
				{
					err = dup2(pipefd[1], 1);
					if (err == -1) perror("dup2 error \n");
				    	close(pipefd[0]);
				    	func(c->u.command[0]);
					_exit(0);
				}
				else 
				{
					err = dup2(pipefd[0],0);
					if (err == -1) perror("dup2 error \n");
					close(pipefd[1]);
					func(c->u.command[1]);
					_exit(0);
				}
			}
			else 
			{
				waitpid(child, NULL, 0);
			}
			break;
		}
		case SEQUENCE_COMMAND:
		{
			pid_t pid = fork();
                        if (pid < 0)
                        {
                                fprintf(stderr, "Cannot create child process \n");
                                exit(1);
	                }
                        else if (pid == 0)
                        {
                                func(c->u.command[0]);
				_exit(0);
        	        }
                        else{
				int status;
                                waitpid(pid, &status, 0);
                                func(c->u.command[1]);
			}
			break;	
		}
		default:
			return 0;
	}
	
	
	return 1;
}

void
execute_command (command_t c, int time_travel)
{
//	printf("%d is executeing \n", getpid());
	pid_t child = fork();
	if (child < 0) 
	{
		fprintf(stderr, "Cannot create child process \n");
        	exit(1);
	}
	else if (child == 0)
	{
//		 printf("the kid %d is executeing \n", getpid());
		func(c);
		_exit(0);
	}
	else
	{
		waitpid(child, NULL, 0);
	}
}

command_t* create_array(command_stream_t cs)
{
	command_t* result = (command_t*)checked_malloc(sizeof(command_t)*50);
	int n = 1;
	// realloc
	command_t tmp;
	while ((tmp = read_command_stream(cs)))
	{
		result[num_stream] = tmp;
		num_stream++;
		if (num_stream == n*50)
		{
			n++;
			result = checked_realloc(result, sizeof(command_t)*n*50); 
		}
	}
	return result;
}

//recursion
void check_io(command_t c, io_struct_t* io)
{
	if(c == NULL) 
		return;
	if (c->type == SIMPLE_COMMAND)
	{
		io_struct_t input = NULL;
		io_struct_t output = NULL;
		if (c->input != NULL)
		{
			input = checked_malloc(sizeof(io_struct_t));
			input->io_type = INPUT;
			input->file_name = c->input;
			input->next = NULL;
		}
		if (c->output != NULL)
		{
			output = checked_malloc(sizeof(io_struct_t));
			output->io_type = OUTPUT;
			output->file_name = c->output;
			output->next = NULL;
		}
		if (input != NULL && output != NULL)
		{
			input->next = output;
			if(*io == NULL) 
				*io = input;
			else {	
				output->next = *io;
				*io = input;
			}
		}
		else if (input != NULL && output == NULL)
		{
			if (*io == NULL) 
				 *io = input;
			else {
				input->next = *io;
				*io = input;
			}
		}
		else if (output != NULL && input == NULL) 
		{
			if (*io == NULL)
			{
				*io = output;}
			else{
				output->next = *io;
				*io = output;
			}
		}
		
	}
	check_io(c->u.command[0], io);
	check_io(c->u.command[1], io);
}

int filename(char* name1, char* name2)
{
	int i;
	for(i = 0; name1[i] != '\0' && name2[i] != '\0'; i++){
        	if(name1[i] != name2[i])
            	return 0;
    	}
    	if(name1[i] != '\0' || name2[i] != '\0')
            return 0;

    	return 1;
}
int check_dependency(io_struct_t io1, io_struct_t io2)
{
	if(filename(io1->file_name, io2->file_name))
		if (io1->io_type != io2->io_type)
		{
			return 1;
		}
		else if (io1->io_type == OUTPUT &&  io2->io_type == OUTPUT)
		{
			return 1;
		}
		else
			return 0;
	else 
		return 0;
}

void print_array(int* graph)
{
	int i, j;
	for (i = 0; i < num_stream; i++)
	{
		for (j = 0; j < num_stream; j++)
			printf("%d ", graph[i*num_stream+j]);
		printf("\n");
	}
}
void print_filename(io_struct_t *array)
{
	int i;
	for (i = 0; i < num_stream; i++)
	{
		io_struct_t tmp = array[i];
		while(tmp!=NULL)
		{
			printf("%c \n", tmp->file_name[0]);
			tmp=tmp->next;
		}
	}
}
int* dependency_graph(command_t* command_array)
{
	int *result = checked_malloc(sizeof(int)*num_stream*num_stream);
	io_struct_t *io_array = checked_malloc(sizeof(io_struct_t)*num_stream);
	int i;
	for (i = 0; i < num_stream; i++)
	{
		io_array[i] = NULL;
		check_io(command_array[i], &(io_array[i]));
	}
	//making dependency graph
	for (i = 0; i < num_stream*num_stream; i++)
		result[i] = 0;
	for (i = 0; i < num_stream; i++)
	{
		io_struct_t tmp = io_array[i];
		while (tmp != NULL)
		{
			int j;
			for (j = i+1; j < num_stream; j++)
			{
				io_struct_t tmp2 = io_array[j];
				while (tmp2 != NULL)
				{
					if (check_dependency(tmp, tmp2))
					{
						result[i*num_stream + j] = 1;
						break;
					}
					tmp2 = tmp2->next;				
				}
			}
			tmp = tmp -> next;
		}
	}
	//print_array(result);
	return result;	
}

int* create_vector(int* dependency)
{
	int *result = checked_malloc(sizeof(int)*num_stream);
	int i, j, sum;
	for (i = 0; i < num_stream; i++)
	{
		sum = 0;
		for(j = 0; j < num_stream; j++)
		{
			sum += dependency[j*num_stream+i];
		}
		result[i] = sum;
	}
	return result;
}

int stupid_check(int *vector)
{
	printf("here  ");
	int i;
	for (i = 0; i < num_stream; i++)
	{
		if(vector[i] > 0)
			return 1;
	}
	return 0;
}
void resolve(command_t* command_array, int* dependency, int* vector)
{
	int num_child = 0; //number of child created to execute each command
	int num_zero = 0; //number of zero in the vector
	int* zero_vector = checked_malloc(sizeof(int)*num_stream); //an array that store the position of zero
	int i;
	while (num_child < num_stream) //before creating enough children to execute all the commands
	{
		for (i = 0; i < num_stream; i++) //count how many zero there are and put the position in the vector
		{
			if (vector[i] == 0) 
			{
				zero_vector[num_zero] = i;
				num_zero++;
				vector[i] = -1; //if it's zero, make it -1 to prevent it from executeing again
			}	
		}
		pid_t pids[num_zero]; //creating num_zero of child process
		for (i = 0; i < num_zero; i++)
		{
			if ((pids[i] = fork()) < 0) {} //error
			else if (pids[i] == 0) 
			{
				//execute the command on the position stored in the zero_vector
				execute_command(command_array[zero_vector[i]], 0);
				_exit(0);					
			}
		}

		//only parent process is supposed to be here
		pid_t pid;
		num_child += num_zero;
		int save = num_zero;
		//wait for all the kids to finish executing 
		while (num_zero > 0)
		{
			pid = waitpid(0, NULL, 0);
			num_zero--;
		}
		//decrement the dependency vector
		for (i = 0; i < save; i++)
		{
			int j;
			for (j = 0; j < num_stream; j++)
			{
				if (dependency[zero_vector[i]*num_stream+j] == 1)
					vector[j] -= 1;
			}
		}
	}
}
 
void timetravel(command_stream_t cs)
{
	command_t *command_array = create_array(cs);	
	int *dependency = dependency_graph(command_array);	
	int *w = create_vector(dependency);
	int i;
	resolve(command_array, dependency, w);
	
	//for (i = 0; i < num_stream; i++) printf("%d ", w[i]);
}
