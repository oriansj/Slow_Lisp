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
#include <stdbool.h>
#include <stdint.h>

FILE* source_file;
bool Reached_EOF;

static struct cell* token_stack;
struct cell* make_sym(char* name);
struct cell* intern(char *name);
struct cell* findsym(char *name);

/****************************************************************
 *      "Convert a string into a list of tokens."               *
 ****************************************************************/
struct cell* tokenize(struct cell* head, char* fullstring, int32_t size)
{
	int32_t i = 0;
	bool done = false;
	if((0 >= size) || (0 == fullstring[0]))
	{
		return head;
	}

	char *store = calloc(max_string + 1, sizeof(char));

	do
	{
		int32_t c = fullstring[i];
		if((i > size) || (max_string <= i))
		{
			done = true;
		}
		else if(34 == c)
		{
			store[i] = c;
			i = i + 1;
			while(34 != fullstring[i])
			{
				store[i] = fullstring[i];
				i = i + 1;
			}
			i = i + 1;
			done = true;
		}
		else
		{
			if((' ' == c) || ('\t' == c) || ('\n' == c) | ('\r' == c))
			{
				i = i + 1;
				done = true;
			}
			else
			{
				store[i] = c;
				i = i + 1;
			}
		}
	} while(!done);

	if(i > 1)
	{
		struct cell* temp = make_sym(store);
		temp->cdr = head;
		head = temp;
	}
	else
	{
		free(store);
	}
	head = tokenize(head, (fullstring+i), (size - i));
	return head;
}


bool is_integer(char* a)
{
	if(('0' <= a[0]) && ('9' >= a[0]))
	{
		return true;
	}

	if('-' == a[0])
	{
		if(('0' <= a[1]) && ('9' >= a[1]))
		{
			return true;
		}
	}

	return false;
}


/********************************************************************
 *     Numbers become numbers                                       *
 *     Strings become strings                                       *
 *     Functions become functions                                   *
 *     quoted things become quoted                                  *
 *     Everything is treated like a symbol                          *
 ********************************************************************/
struct cell* atom(struct cell* a)
{
	/* Check for quotes */
	if('\'' == a->string[0])
	{
		a->string = a->string + 1;
		return make_cons(quote, make_cons(a, nil));
	}

	/* Check for strings */
	if(34 == a->string[0])
	{
		a->type = STRING;
		a->string = a->string + 1;
		return a;
	}

	/* Check for integer */
	if(is_integer(a->string))
	{
		a->type = INT;
		a->value = strtol(a->string, NULL, 0);
		return a;
	}

	/* Check for functions */
	struct cell* op = findsym(a->string);
	if(nil != op)
	{
		return op->car;
	}

	/* Assume new symbol */
	all_symbols = make_cons(a, all_symbols);
	return a;
}

/****************************************************************
 *     "Read an expression from a sequence of tokens."          *
 ****************************************************************/
struct cell* readlist();
struct cell* readobj()
{
	struct cell* head = token_stack;
	token_stack = head->cdr;
	head->cdr = NULL;
	if (! strncmp("(", head->string, max_string))
	{
		return readlist();
	}

	return atom(head);
}

struct cell* readlist()
{
	struct cell* head = token_stack;
	if (! strncmp(")", head->string, max_string))
	{
		token_stack = head->cdr;
		return nil;
	}

	struct cell* tmp = readobj();
//	token_stack = head->cdr;
	return make_cons(tmp,readlist());
}

/****************************************************
 *     Put list of tokens in correct order          *
 ****************************************************/
struct cell* reverse_list(struct cell* head)
{
	struct cell* root = NULL;
	while(NULL != head)
	{
		struct cell* next = head->cdr;
		head->cdr = root;
		root = head;
		head = next;
	}
	return root;
}

/****************************************************
 *     "Read a Scheme expression from a string."    *
 ****************************************************/
struct cell* parse(char* program, int32_t size)
{
	token_stack = tokenize(NULL, program, size);
	if(NULL == token_stack)
	{
		return nil;
	}
	token_stack = reverse_list(token_stack);
	return readobj();
}

/****************************************************
 * Do the heavy lifting of reading an s-expreesion  *
 ****************************************************/
uint32_t Readline(FILE* source_file, char* temp)
{
	char store[max_string + 2] = {0};
	int32_t c;
	uint32_t i;
	uint32_t depth = 0;

	for(i = 0; i < max_string; i = i + 1)
	{
restart_comment:
		c = fgetc(source_file);
		if((-1 == c) || (4 == c))
		{
			return i;
		}
		else if(59 == c)
		{
			/* drop everything until we hit newline */
			while(10 != c)
			{
				c = fgetc(source_file);
			}
			goto restart_comment;
		}
		else if(34 == c)
		{ /* Deal with strings */
			store[i] = (char)c;
			i = i + 1;
			c = fgetc(source_file);
			while(34 != c)
			{
				store[i] = (char)c;
				i = i + 1;
				c = fgetc(source_file);
			}
			store[i] = (char)c;
		}
		else if((0 == depth) && ((10 == c) || (13 == c) || (32 == c) || (9 == c)))
		{
			goto Line_complete;
		}
		else if(('(' == c) || (')' == c))
		{
			if('(' == c)
			{
				depth = depth + 1;
			}

			if(')' == c)
			{
				depth = depth - 1;
			}

			store[i] = ' ';
			store[i+1] = c;
			store[i+2] = ' ';
			i = i + 2;
		}
		else
		{
			store[i] = (char)c;
		}
	}

Line_complete:
	if(1 > i)
	{
		return Readline(source_file, temp);
	}

	strncpy(temp, store, max_string);
	return i;
}
