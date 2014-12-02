// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>
#include "alloc.h"
#include <error.h>

#include<ctype.h>

int stream_size = 1;
int counter = 0;
int line_number = 1;
int paren_number = 0;

struct command_stream
{
	command_t command;
	command_stream_t next;
};

int isOperator (char c)
{
	if (c == '&' || c == ';' || c == '|'){
		return 1;
	}
	return 0;
}

int isWhiteSpace (char c)
{
	if (c == ' ' || c == '\t'){
		return 1;
	}
	return 0;
}

int isValid (char c){
	return(c == '!' || c == '%' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_' || isdigit((int) c) 
	|| isalpha((int) c) || isWhiteSpace(c) || 
	isOperator(c) || c == '(' || c == ')' || c == '<' || c == '>' || c == '#' || c == '\n' || c == EOF);
}
//reads entire command until operators
char* read_token (int (*get_next_byte) (void *),
         void *get_next_byte_argument, int *pos, int check_newline)
{
	char c;
	int tmp_pos = *pos;
	int tmpSize = 50;
	char *token = (char*)checked_malloc(sizeof(char)*tmpSize);
	c = get_next_byte(get_next_byte_argument);
	char *paren = (char*)checked_malloc(sizeof(char));
	if(!isValid(c)){
		fprintf(stderr, "%d: Invalid Syntax", line_number);
		exit(1);
	}

	if(check_newline == 0)
	{
		while(!isOperator(c) && c != EOF && c != '\n' && c !=')')
		{
			if (c == '('){
				paren[0] = '(';
				return paren;
			}
			if(tmp_pos > tmpSize){
				tmpSize += 50;
				token = (char*) checked_realloc(token, sizeof(char)*tmpSize);
			}
  			token[tmp_pos] = c;
  			tmp_pos++;
			c = get_next_byte(get_next_byte_argument);
			if(!isValid(c)){
		                fprintf(stderr, "%d: Invalid Syntax", line_number);
                		exit(1);
        		}
		}
	}		
	else
	{
		while(!isOperator(c) && c != EOF && c != ')')
  		{
			if (c == '(')
  			{
				paren[0] = '(';
				return paren;
  			}
			if(tmp_pos > tmpSize){
                                tmpSize += 50;
                                token = (char*) checked_realloc(token, sizeof(char)*tmpSize);
                        }
			
			if (c == '\n') line_number++;
  			token[tmp_pos] = c;
  			tmp_pos++;
			c = get_next_byte(get_next_byte_argument);
			if(!isValid(c)){
                                fprintf(stderr, "%d: Invalid Syntax", line_number);
                                exit(1);
                        }
		}
	}
  	if(c == '\n'){
		line_number++;
	}
	if (c == ')') paren_number--;

	if (paren_number < 0)
	{
		fprintf(stderr, "%d: Invalid Syntax", line_number);
                exit(1);
	}
	if(!isValid(c)){
                fprintf(stderr, "%d: Invalid Syntax", line_number);
                exit(1);
        }
	 if(tmp_pos > tmpSize){
              tmpSize += 50;
              token = (char*) checked_realloc(token, sizeof(char)*tmpSize);
        }

  	token[tmp_pos] = c;
  	*pos = tmp_pos;
  	return token;
}

//auxiliary function for make_command
int isEndOfWord(char c){
    if( isWhiteSpace(c) || c == '\0'){
        return 1;
    } else {
        return 0;
    }
}


//auxiliary function for make_command
int getNextWord(int start, char *token, char **word){
	int bufferSize = 50;
	int i;
	(*word)[0] = '\0';
	for(i = 0; !isEndOfWord(token[start]); i++){
		if(i > bufferSize){
			bufferSize += 10;
			*word = checked_realloc(word, bufferSize);
		}
		(*word)[i] = token[start];
		start++;
		//test 4
		if((*word)[i] == '>' || (*word)[i] == '<'){
			break;
		}
	}
	return start;
}

//auxiliary function purely to grab input/output files
int getInputOutput(int start, char *token, char**word){
	while(isWhiteSpace(token[start])){
		start++;
	}
	if(isOperator(token[start]) || token[start] == '\0'){
        	fprintf(stderr, "%d: Incorrect Syntax1 \n", line_number);
        	exit(1);
	}
	int bufferSize = 50;
	int i = 0;
	while(!isWhiteSpace(token[start]) && token[start]!='\0'){
		 if(i > bufferSize){
            bufferSize += 50;
            *word = checked_realloc(word, bufferSize);
        }
        (*word)[i] = token[start];
			start++;
			i++;
	}
	(*word)[i] = '\0';
	return start;
}

//used for debugging purposes
void printWord(char* word){
	int i;
	for(i = 0; word[i] != '\0' && word[i] != EOF; i++){
		printf("%c", word[i]);
	}
}

int containsCarrot(char *token){
	if(token == NULL){
		return 0;
	}
	int inCarrot = 0;
	int outCarrot = 0;
	int i;
	for(i = 0; token[i] != '\0' && token[i] != EOF ; i++){
		if(token[i] == '<'){
			inCarrot++;
		}
		else if( token[i] == '>'){
			outCarrot++;
		}
	}
	if(inCarrot > 1 || outCarrot > 1){
		fprintf(stderr, "%d: Incorrect Syntax\n", line_number);
		exit(1);
	}
	return(inCarrot || outCarrot);
}

//creates the command struct based on tokens passed in
command_t make_command(char *token, command_t subcommand)
{	
	//checks for more than one <, >
	containsCarrot(token);
	command_t cmdStruct = checked_malloc(sizeof(struct command));
	//for subshell
	if(token == NULL){
		cmdStruct->type = SUBSHELL_COMMAND;
		cmdStruct->input = 0;
		cmdStruct->output = 0;
		cmdStruct->u.subshell_command = subcommand;
		return cmdStruct;
	}
	int tokenTracker = 0;
	//if token starts with spaces...
	int i;
	for(i = 0; isWhiteSpace(token[i]); i++){        
		tokenTracker++;
	}
	int start = tokenTracker;
	char* command = checked_malloc(50);
	tokenTracker = getNextWord(tokenTracker, token, &command);
	if(command[0] == '&'){
		cmdStruct->type = AND_COMMAND;
		cmdStruct->input = 0;
		cmdStruct->output = 0;
	} else if (command[0] == '|'){
		if(command[1] == '|'){
			cmdStruct->type = OR_COMMAND;
		} else {
            cmdStruct->type = PIPE_COMMAND;
		}
		cmdStruct->input = 0;
		cmdStruct->output = 0;
	} else if (command[0] == ';'){
		cmdStruct->type = SEQUENCE_COMMAND;
		cmdStruct->input = 0;
		cmdStruct->output = 0;
    } else if (command[0] == '<' || command[0] == '>'){
		fprintf(stderr, "%d: Incorrect syntax2 \n", line_number);
		exit(1);
	} else {
			cmdStruct->type = SIMPLE_COMMAND;
			int bufferSize = 10;
			char** wordArray = checked_malloc(sizeof(char*)*10);
			int numberOfWords = 0;
			wordArray[numberOfWords] = checked_malloc(tokenTracker+ 1); 
			command[tokenTracker] = '\0';
			if(containsCarrot(command)){
				 int buffSize = 50;
                 char* com = checked_malloc(buffSize);
                 int comstart = 0;
                 while(command[comstart] != '<' && command[comstart]!= '>'){
						if(comstart >= buffSize){
							buffSize += 50;
							com = checked_realloc(com, sizeof(char*)*buffSize);
						}
                        com[comstart] = command[comstart];
                        comstart++;
                 }
				 wordArray[numberOfWords] = com;
				tokenTracker = tokenTracker - 2 ;
            } else{
				wordArray[numberOfWords] = command;
			}
			numberOfWords++;
			while(token[tokenTracker] != '\0'){
					char* word = checked_malloc(50);
					tokenTracker++;
					start = tokenTracker;
					tokenTracker = getNextWord(tokenTracker, token, &word);
					if(word[0] != '<' && word[0] != '>' && (word[tokenTracker-start-1] == '<' || word[tokenTracker-start-1]  == '>')){
						word[tokenTracker-start-1] = '\0';
						tokenTracker = tokenTracker-2; 
					} else{	
						word[tokenTracker-start] = '\0';
					}
				if(word[0] == '<'){
		 			char* in = checked_malloc(50);
					tokenTracker = getInputOutput(tokenTracker, token, &in); 
					cmdStruct->input = in;
            	} else if (word[0] == '>'){
		 			char* output = checked_malloc(50);
					tokenTracker = getInputOutput(tokenTracker, token, &output);
                	cmdStruct->output = output;
            	} else {
					if(numberOfWords >= bufferSize){
                	bufferSize += 50;
                	wordArray = checked_realloc(wordArray, sizeof(char*)*bufferSize);
					}
               		if(word[0] != '\0'){        
                    	wordArray[numberOfWords] = checked_malloc(tokenTracker-start);
                    	wordArray[numberOfWords] = word;
                    	numberOfWords++;
						cmdStruct->input = 0;
						cmdStruct->output = 0;
               		}
            	}
        }
        wordArray[numberOfWords] = '\0';
        cmdStruct->u.word = wordArray;
	}
	return cmdStruct;
}

//auxiliary function for reordering the tree for | operators
command_t reorder(command_t cmd1, command_t pipe, command_t cmd2)
{
	pipe->u.command[0]=cmd1;
	pipe->u.command[1]=cmd2;
	return pipe;
}

//takes in 3 command structs and links them into a tree
command_t create_command_tree(command_t cmd1, command_t operator, command_t cmd2)
{
	command_t tmp;

	if (operator->type == PIPE_COMMAND && cmd1->type != SIMPLE_COMMAND) {
		tmp = reorder(cmd1->u.command[1], operator, cmd2);
		cmd1->u.command[1] = tmp;
		return cmd1;
	} else { 
		operator->u.command[0]=cmd1;
		operator->u.command[1]=cmd2;
		return operator;
	}
}

//returns an entire command stream; points to head of tree
//grabs command struct 1, operator, command struct 2 and makes them a tree
command_t make_single_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument, int *flag)
{
	command_t a;
	command_t b;
	command_t o;
	int position = 0;
	int check_newline = 0;

	char *token_a = read_token(get_next_byte, get_next_byte_argument, &position, check_newline);
	//check for comments, if whiteSpace in front of comment
	int i = 0;
    while(isWhiteSpace(token_a[i])){
        i++;
    }
	while (token_a[0] == '\n' || token_a[0] == '#')
	{
        position = 0;
        token_a = read_token(get_next_byte, get_next_byte_argument, &position, check_newline);	
	}

	if (token_a[position] == EOF){ 
		*flag = 1; 
		return 0;
	}
	//Checking if token_a is an operator
	int k = 0;
	while(isWhiteSpace(token_a[k])){
		k++;
	}
	if(isOperator(token_a[k])){
		fprintf(stderr, "%d: Incorrect Syntax1 \n", line_number);
		exit(1);
	}
	char op;
	char *operator = (char*)checked_malloc(5);
	if (token_a[position] == '(')
	{
		paren_number++;
		a = make_single_command_stream(get_next_byte, get_next_byte_argument, flag);
		a = make_command(NULL, a);
		operator = read_token(get_next_byte, get_next_byte_argument, &position, 0);
	} else {
		operator[0] = token_a[position];
  		token_a[position] = '\0';
		a = make_command(token_a, NULL);	
	}

 	while (*operator != EOF && *operator != ')')
	{
		k = 0;
		while(isWhiteSpace(operator[k])){
			k++;
		}
		operator += k;

		if (paren_number == 0 && *operator == ';')
			break;
		
		if (paren_number == 0 && *operator == '\n')
			break;
	
		position = 0;
		if (operator[0] == '&') {
			operator[1] = get_next_byte(get_next_byte_argument);
 			if(operator[1] == ';'){
				fprintf(stderr, "%d: Incorrect Syntax2 \n", line_number);
				exit(1);
        		}else if(operator[1] != '&')
			{
				fprintf(stderr, "%d: Incorrect Syntax3 \n", line_number);
                                exit(1);
			}
			operator[2] = '\0';
  		} else if (operator[0] == '|') {
			op = get_next_byte(get_next_byte_argument);
			if (op == '|'){
				op == '\0';
				operator[1] = '|';
				operator[2] = '\0';
			} else if( op == ';'){
		 		fprintf(stderr, "%d: Incorrect Syntax4 \n", line_number);
                exit(1);
			} else {
				operator[1] = '\0';
			}
  		} else if(operator[0] == ';' || operator[0] == EOF) {
			operator[1] = '\0';
		}  else if(operator[0] == '\n'){
			operator[0] = ';';
			operator[1] = '\0';
		}
		o = make_command(operator, NULL);

		char *token_b = read_token(get_next_byte, get_next_byte_argument, &position, check_newline);
		//check for #, even ones that start with spaces in front of #

		while (position == 0 && check_newline == 0 && token_b[position] == '\n' && o->type != SEQUENCE_COMMAND)
		{
			token_b = read_token(get_next_byte, get_next_byte_argument, &position, check_newline);
		}			 
 
		if (o->type == PIPE_COMMAND && op != '\n') {
			int i = 0;
			char store = token_b[0];
			token_b[0] = op;
			op = token_b[1];
			i++;
			while (i <= position)
			{
				token_b[i] = store;
				store = op;	
				i++;
				op = token_b[i];
			}
			position++;
			token_b[position] = store;
  		}
		//check token_b for operator or EOF
		int  w =0;
		while(isWhiteSpace(token_b[w])){
			w++;
  		}
		if(isOperator(token_b[w]) || token_b[w] == EOF){
			fprintf(stderr, "%d: Incorrect Syntax6 \n", line_number);
			exit(1);
		}	

		if (token_b[position] == '(') {
			paren_number++;
			b = make_single_command_stream(get_next_byte, get_next_byte_argument, flag);
			b = make_command(NULL, b);
			operator = read_token(get_next_byte, get_next_byte_argument, &position, check_newline);
  		} else {
			if (token_b[position-1] == '\n') {
  				operator[0] = token_b[position-1];
				token_b[position-1] = '\0';
			} else {
				operator[0] = token_b[position];
				token_b[position] = '\0';
			}
			b = make_command(token_b, NULL);	
  		}
  		a = create_command_tree(a, o, b);
	}

	if(*operator == EOF) 
		*flag = 1;
	if(paren_number != 0) 
	{
		fprintf(stderr, "%d: Invalid Syntax7", line_number);
		exit(1);
	}
  	return a;
}

void add_to_command_stream_list (command_stream_t command_stream_head, command_stream_t command_stream_node)
{
	if (command_stream_node != NULL){
	command_stream_t tmp = command_stream_head;
	while(tmp->next != NULL)
		tmp = tmp->next;
	tmp->next = command_stream_node;
	stream_size++;}
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	command_stream_t head = checked_malloc(sizeof(struct command_stream));
	command_stream_t traverse;
	int flag;
	flag = 0;
	head->command = make_single_command_stream(get_next_byte, get_next_byte_argument, &flag);
  	head->next = NULL;
	while (flag != 1)
	{
		traverse = checked_malloc(sizeof(struct command_stream));
		traverse->command = make_single_command_stream(get_next_byte, get_next_byte_argument, &flag);
		if (traverse->command == 0) {break;}
		traverse->next = NULL;
		add_to_command_stream_list(head, traverse);
	}

  return head;
}


command_t
read_command_stream (command_stream_t s)
{
	//printf("stream size is %d and counter size is %d \n", stream_size, counter);
	if (counter == stream_size) return 0;
	
	command_stream_t start = s;
	int i = 0;
	while (i < counter)
	{
		start = start->next;
		i++;
	}
	if (counter < stream_size)
		counter++;

	command_t tmp = start->command;
	return tmp;
}
