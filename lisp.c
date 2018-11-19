/* Copyright (C) 2016 Jeremiah Orians
 * This file is part of stage0.
 *
 * stage0 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * stage0 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with stage0.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lisp.h"
// CONSTANT MAX_STRING 4096
#define MAX_STRING 4096

struct file_list
{
	struct file_list* next;
	FILE* file;
};

char* message;

/* Prototypes */
struct cell* eval(struct cell* exp, struct cell* env);
void init_sl3();
int Readline(FILE* source_file, char* temp, unsigned max_string);
struct cell* parse(char* program, int size);
void writeobj(FILE *ofp, struct cell* op);
void garbage_init(int number_of_cells);
void garbage_collect();
void reset_block(char* a);

/* Read Eval Print Loop*/
int REPL(FILE* in, FILE *out)
{
	int read;
	input = in;
	/* Reset contents of message */
	reset_block(message);
	read = Readline(in, message, MAX_STRING);
	if(0 == read)
	{
		return TRUE;
	}
	struct cell* temp = parse(message, read);
	current = temp;
	temp = eval(temp, top_env);
	writeobj(out, temp);
	current = nil;
	if(echo) fputc('\n', out);
	return FALSE;
}

void recursively_evaluate(struct file_list* a)
{
	if(NULL == a) return;
	recursively_evaluate(a->next);
	int Reached_EOF = FALSE;
	while(!Reached_EOF)
	{
		garbage_collect();
		Reached_EOF = REPL(a->file, console_output);
	}
}

FILE* open_file(char* name, char* mode)
{
	FILE* f = fopen(name, mode);

	if(NULL == f)
	{
		file_print("Unable to open file ", stderr);
		file_print(name, stderr);
		if('r' == mode[0])
		{
			file_print(" for reading\n", stderr);
		}
		else if('w' == mode[0])
		{
			file_print(" for writing\n", stderr);
		}
		else
		{
			file_print(" with unknown mode\n", stderr);
		}
		exit(EXIT_FAILURE);
	}

	return f;
}

/*** Main Driver ***/
int main(int argc, char **argv)
{
	int number_of_cells = 1000000;
	file_output = fopen("/dev/null", "w");
	console_output = stdout;
	struct file_list* essential = NULL;

	int i = 1;
	while(i <= argc)
	{
		if(NULL == argv[i])
		{
			i = i + 1;
		}
		else if(match(argv[i], "-c") || match(argv[i], "--console"))
		{
			console_output = open_file(argv[i + 1], "w");
			i = i + 2;
		}
		else if(match(argv[i], "-f") || match(argv[i], "--file"))
		{
			struct file_list* new = calloc(1, sizeof(struct file_list));
			new->file = open_file(argv[i + 1], "r");
			new->next = essential;
			essential = new;
			i = i + 2;
		}
		else if(match(argv[i], "-h") || match(argv[i], "--help"))
		{
			file_print("Usage: ", stdout);
			file_print(argv[0], stdout);
			file_print(" -f FILENAME1 {-f FILENAME2}\n", stdout);
			exit(EXIT_SUCCESS);
		}
		else if(match(argv[i], "-m") || match(argv[i], "--memory"))
		{
			number_of_cells = numerate_string(argv[i + 1]);
			i = i + 2;
		}
		else if(match(argv[i], "-o") || match(argv[i], "--output"))
		{
			file_output = open_file(argv[i + 1], "w");
			i = i + 2;
		}
		else if(match(argv[i], "-v") || match(argv[i], "--version"))
		{
			file_print("Slow_Lisp 0.1\n", stdout);
			exit(EXIT_SUCCESS);
		}
		else
		{
			file_print("Unknown option\n", stderr);
			exit(EXIT_FAILURE);
		}
	}

	/* Our most important initializations */
	memory_block = calloc(MAX_STRING, sizeof(char));
	message = calloc(MAX_STRING + 2, sizeof(char));
	garbage_init(number_of_cells);
	init_sl3();
	int Reached_EOF;
	echo = TRUE;

	recursively_evaluate(essential);

	Reached_EOF = FALSE;
	while(!Reached_EOF)
	{
		garbage_collect();
		Reached_EOF = REPL(stdin, stdout);
	}
	fclose(file_output);
	return 0;
}
