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
#include <getopt.h>
#include <stdint.h>

struct file_list
{
	struct file_list* next;
	FILE* file;
};

/* Prototypes */
struct cell* eval(struct cell* exp, struct cell* env);
void init_sl3();
uint32_t Readline(FILE* source_file, char* temp);
struct cell* parse(char* program, int32_t size);
void writeobj(FILE *ofp, struct cell* op);
void garbage_init(int number_of_Cells);
void garbage_collect();

/* Read Eval Print Loop*/
bool REPL(FILE* in, FILE *out)
{
	int read;
	input = in;
	char* message = calloc(max_string + 2, sizeof(char));
	read = Readline(in, message);
	if(0 == read)
	{
		return true;
	}
	struct cell* temp = parse(message, read);
	current = temp;
	temp = eval(temp, top_env);
	writeobj(out, temp);
	current = nil;
	if(echo) fprintf(out, "\n");
	return false;
}

void recursively_evaluate(struct file_list* a)
{
	if(NULL == a) return;
	recursively_evaluate(a->next);
	bool Reached_EOF = false;
	while(!Reached_EOF)
	{
		garbage_collect();
		Reached_EOF = REPL(a->file, console_output);
	}
}

/*** Main Driver ***/
int main(int argc, char **argv)
{
	int number_of_cells = 1000000;
	file_output = fopen("tape_02", "w");
	console_output = stdout;
	struct file_list* essential;
	static struct option long_options[] = {
		{"console", required_argument, 0, 'c'},
		{"file", required_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{"memory", required_argument, 0, 'm'},
		{"output", required_argument, 0, 'o'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int c;
	int option_index = 0;
	while ((c = getopt_long(argc, argv, "c:f:h:m:o:v", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 0: break;
			case 'c':
			{
				console_output =  fopen(optarg, "w");
				break;
			}
			case 'f':
			{
				struct file_list* new = calloc(1, sizeof(struct file_list));
				new->file = fopen(optarg, "r");
				new->next = essential;
				essential = new;
				break;
			}
			case 'h':
			{
				fprintf(stderr, "Usage: %s -f FILENAME1 {-f FILENAME2}\n", argv[0]);
				exit(EXIT_SUCCESS);
			}
			case 'm':
			{
//				number_of_cells = strtol(optarg, NULL,  0);
				break;
			}
			case 'o':
			{
				file_output =  fopen(optarg, "w");
				break;
			}
			case 'v':
			{
				fprintf(stdout, "Slow_Lisp 0.1\n");
				exit(EXIT_SUCCESS);
			}
			default:
			{
				fprintf(stderr, "Unknown option\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	/* Our most important initializations */
	garbage_init(number_of_cells);
	init_sl3();
	bool Reached_EOF;
	echo = true;

	recursively_evaluate(essential);

	Reached_EOF = false;
	while(!Reached_EOF)
	{
		garbage_collect();
		Reached_EOF = REPL(stdin, stdout);
	}
	fclose(file_output);
	return 0;
}
