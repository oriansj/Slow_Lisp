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

/* Prototypes */
struct cell* eval(struct cell* exp, struct cell* env);
void init_sl3();
uint32_t Readline(FILE* source_file, char* temp);
struct cell* parse(char* program, int32_t size);
void writeobj(FILE *ofp, struct cell* op);
void garbage_init();
void garbage_collect();

/* Read Eval Print Loop*/
bool REPL(FILE* in, FILE *out)
{
	int read;
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
	if(echo) printf("\n");
	return false;
}


/*** Main Driver ***/
int main(int argc, char **argv)
{
	/* Our most important initializations */
	garbage_init();
	init_sl3();
	bool Reached_EOF;
	echo = true;
	output = fopen("tape_02", "w");

	static struct option long_options[] = {
		{"file", required_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int c;
	int option_index = 0;
	while ((c = getopt_long(argc, argv, "f:h:v", long_options, &option_index)) != -1)
	{
		switch(c)
		{
			case 0: break;
			case 'h':
			{
				fprintf(stderr, "Usage: %s -f FILENAME1 {-f FILENAME2}\n", argv[0]);
				exit(EXIT_SUCCESS);
			}
			case 'f':
			{
				FILE* source_file = fopen(optarg, "r");
				Reached_EOF = false;
				while(!Reached_EOF)
				{
					garbage_collect();
					Reached_EOF = REPL(source_file, stdout);
				}
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

	Reached_EOF = false;
	while(!Reached_EOF)
	{
		garbage_collect();
		Reached_EOF = REPL(stdin, stdout);
	}
	fclose(output);
	return 0;
}
