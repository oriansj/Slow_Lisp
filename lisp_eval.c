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
/* Imported functions */
struct cell* prim_output(struct cell* args, FILE* out);
FILE* open_file(char* name, char* mode);
void garbage_collect();
struct cell* make_cell(int type, struct cell* a, struct cell* b, struct cell* env);
struct cell* make_char(int a);
struct cell* make_int(int a);
struct cell* make_file(FILE* a);
struct cell* make_prim(void* fun);
struct cell* make_sym(char* name);
struct cell* make_string(char* a);
struct cell* make_sym(char* name);
struct cell* make_proc(struct cell* a, struct cell* b, struct cell* env);
struct cell* make_eof();

/* Support functions */
struct cell* findsym(char *name)
{
	struct cell* symlist;
	for(symlist = all_symbols; nil != symlist; symlist = symlist->cdr)
	{
		if(match(name, symlist->car->string))
		{
			return symlist;
		}
	}
	return nil;
}

struct cell* intern(char *name)
{
	struct cell* op = findsym(name);
	if(nil != op) return op->car;
	op = make_sym(name);
	all_symbols = make_cons(op, all_symbols);
	return op;
}

/*** Environment ***/
struct cell* extend(struct cell* env, struct cell* symbol, struct cell* value)
{
	return make_cons(make_cons(symbol, value), env);
}

struct cell* multiple_extend(struct cell* env, struct cell* syms, struct cell* vals)
{
	if(nil == syms)
	{
		return env;
	}
	return multiple_extend(extend(env, syms->car, vals->car), syms->cdr, vals->cdr);
}

struct cell* extend_env(struct cell* sym, struct cell* val, struct cell* env)
{
	env->cdr = make_cons(env->car, env->cdr);
	env->car = make_cons(sym, val);
	return val;
}

struct cell* assoc(struct cell* key, struct cell* alist)
{
	if(nil == alist) return nil;
	for(; nil != alist; alist = alist->cdr)
	{
		if(alist->car->car->string == key->string) return alist->car;
	}
	return nil;
}

/*** Evaluator (Eval/Apply) ***/
struct cell* eval(struct cell* exp, struct cell* env);
struct cell* evlis(struct cell* exps, struct cell* env)
{
	if(exps == nil) return nil;
	return make_cons(eval(exps->car, env), evlis(exps->cdr, env));
}

struct cell* progn(struct cell* exps, struct cell* env)
{
	if(exps == nil) return nil;

	struct cell* result;
progn_reset:
	result = eval(exps->car, env);
	if(exps->cdr == nil) return result;
	exps = exps->cdr;
	goto progn_reset;
}

struct cell* exec_func(FUNCTION * func, struct cell* vals)
{
	return func(vals);
}

struct cell* apply(struct cell* proc, struct cell* vals)
{
	struct cell* temp = nil;
	if(proc->type == PRIMOP)
	{
		temp = exec_func(proc->function, vals);
	}
	else if(proc->type == PROC)
	{
		struct cell* env = make_cons(proc->env->car, proc->env->cdr);
		temp = progn(proc->cdr, multiple_extend(env, proc->car, vals));
	}
	else
	{
		file_print("Bad argument to apply\n", stderr);
		exit(EXIT_FAILURE);
	}
	return temp;
}

struct cell* evcond(struct cell* exp, struct cell* env)
{
	/* Return nil but the result is technically undefined per the standard */
	if(nil == exp)
	{
		return nil;
	}

	if(tee == eval(exp->car->car, env))
	{
		return eval(exp->car->cdr->car, env);
	}

	return evcond(exp->cdr, env);
}

struct cell* evwhile(struct cell* exp, struct cell* env)
{
	if(nil == exp) return nil;
	struct cell* conditional = eval(exp->cdr->car, env);

	while(tee == conditional)
	{
		eval(exp->cdr->cdr->car, env);
		conditional = eval(exp->cdr->car, env);
		if((tee == exp->cdr->car) && (left_to_take < 1000)) garbage_collect();
	}

	return conditional;
}

struct cell* process_sym(struct cell* exp, struct cell* env);
struct cell* process_cons(struct cell* exp, struct cell* env);

struct cell* eval(struct cell* exp, struct cell* env)
{
	if(exp == nil) return nil;
	if(SYM == exp->type) return process_sym(exp, env);
	if(CONS == exp->type) return process_cons(exp, env);
	return exp;
}

struct cell* process_sym(struct cell* exp, struct cell* env)
{
	struct cell* tmp = assoc(exp, env);
	if(tmp == nil)
	{
		file_print("Unbound symbol:", stderr);
		file_print(exp->string, stderr);
		fputc('\n', stderr);
		exit(EXIT_FAILURE);
	}
	return tmp->cdr;
}

struct cell* process_if(struct cell* exp, struct cell* env)
{
	if(eval(exp->cdr->car, env) != nil)
	{
		return eval(exp->cdr->cdr->car, env);
	}
	return eval(exp->cdr->cdr->cdr->car, env);

}

struct cell* process_setb(struct cell* exp, struct cell* env)
{
	struct cell* newval = eval(exp->cdr->cdr->car, env);
	struct cell* pair = assoc(exp->cdr->car, env);
	pair->cdr = newval;
	return newval;
}

struct cell* process_let(struct cell* exp, struct cell* env)
{
	struct cell* lets;
	for(lets = exp->cdr->car; lets != nil; lets = lets->cdr)
	{
		env = make_cons(make_cons(lets->car->car, eval(lets->car->cdr->car, env)), env);
	}
	return progn(exp->cdr->cdr, env);
}

struct cell* process_cons(struct cell* exp, struct cell* env)
{
	if(exp->car == s_if) return process_if(exp, env);
	if(exp->car == s_cond) return evcond(exp->cdr, env);
	if(exp->car == s_begin) return progn(exp->cdr, env);
	if(exp->car == s_lambda) return make_proc(exp->cdr->car, exp->cdr->cdr, env);
	if(exp->car == quote) return exp->cdr->car;
	if(exp->car == s_define) return(extend_env(exp->cdr->car, eval(exp->cdr->cdr->car, env), env));
	if(exp->car == s_setb) return process_setb(exp, env);
	if(exp->car == s_let) return process_let(exp, env);
	if(exp->car == s_while) return evwhile(exp, env);
	return apply(eval(exp->car, env), evlis(exp->cdr, env));
}


/*** Primitives ***/
struct cell* prim_apply(struct cell* args)
{
	return apply(args->car, args->cdr->car);
}

struct cell* prim_atom(struct cell* args)
{
	if(CONS == args->car->type) return nil;
	return tee;
}

struct cell* nullp(struct cell* args)
{
	if(nil == args->car) return tee;
	return nil;
}

struct cell* prim_eofp (struct cell* args)
{
	if(EOF_object == args->car->type) return tee;
	return nil;
}

struct cell* prim_sum(struct cell* args)
{
	if(nil == args) return nil;

	int sum;
	for(sum = 0; nil != args; args = args->cdr)
	{
		sum = sum + args->car->value;
	}
	return make_int(sum);
}

struct cell* prim_sub(struct cell* args)
{
	if(nil == args) return nil;

	int sum = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		 sum = sum - args->car->value;
	}
	return make_int(sum);
}

struct cell* prim_prod(struct cell* args)
{
	if(nil == args) return nil;

	int prod;
	for(prod = 1; nil != args; args = args->cdr)
	{
		prod = prod * args->car->value;
	}
	return make_int(prod);
}

struct cell* prim_div(struct cell* args)
{
	if(nil == args) return make_int(1);

	int div = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		div = div / args->car->value;
	}
	return make_int(div);
}

struct cell* prim_mod(struct cell* args)
{
	if(nil == args) return nil;

	int mod = args->car->value % args->cdr->car->value;
	if(nil != args->cdr->cdr)
	{
		file_print("wrong number of arguments to mod\n", stderr);
		exit(EXIT_FAILURE);
	}
	return make_int(mod);
}

struct cell* prim_and(struct cell* args)
{
	if(nil == args) return nil;

	for(; nil != args; args = args->cdr)
	{
		if(tee != args->car) return nil;
	}
	return tee;
}

struct cell* prim_or(struct cell* args)
{
	if(nil == args) return nil;

	for(; nil != args; args = args->cdr)
	{
		if(tee == args->car) return tee;
	}
	return nil;
}

struct cell* prim_not(struct cell* args)
{
	if(nil == args) return nil;

	if(tee != args->car) return tee;
	return nil;
}

struct cell* prim_numgt(struct cell* args)
{
	if(nil == args) return nil;

	int temp = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(temp <= args->car->value)
		{
			return nil;
		}
		temp = args->car->value;
	}
	return tee;
}

struct cell* prim_numge(struct cell* args)
{
	if(nil == args) return nil;

	int temp = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(temp < args->car->value)
		{
			return nil;
		}
		temp = args->car->value;
	}
	return tee;
}

struct cell* prim_numeq(struct cell* args)
{
	if(nil == args) return nil;

	int temp = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(temp != args->car->value)
		{
			return nil;
		}
	}
	return tee;
}

struct cell* prim_numle(struct cell* args)
{
	if(nil == args) return nil;

	int temp = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(temp > args->car->value)
		{
			return nil;
		}
		temp = args->car->value;
	}
	return tee;
}

struct cell* prim_numlt(struct cell* args)
{
	if(nil == args) return nil;

	int temp = args->car->value;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(temp >= args->car->value)
		{
			return nil;
		}
		temp = args->car->value;
	}
	return tee;
}

struct cell* prim_listp(struct cell* args)
{
	if(nil == args) return nil;

	if(CONS == args->car->type)
	{
		return tee;
	}
	return nil;
}

struct cell* prim_get_type(struct cell* args)
{
	if(nil == args) return nil;
	return make_int(args->car->type);
}

struct cell* prim_set_type(struct cell* args)
{
	if(nil == args) return nil;
	return make_cell(args->cdr->car->value, args->car->car, args->car->cdr, args->car->env);
}

struct cell* prim_stringeq(struct cell* args)
{
	if(nil == args) return nil;

	char* temp = args->car->string;
	for(args = args->cdr; nil != args; args = args->cdr)
	{
		if(!match(temp, args->car->string))
		{
			return nil;
		}
	}
	return tee;
}

struct cell* prim_display(struct cell* args)
{
	return prim_output(args, console_output);
}

struct cell* prim_write(struct cell* args)
{
	if(nil == args->cdr)
	{
		return prim_output(args, file_output);
	}
	else if(FILE_PORT == args->cdr->car->type)
	{
		return prim_output(args, args->cdr->car->file);
	}

	file_print("You passed something that isn't a file pointer to write in position 2\n", stderr);
	exit(EXIT_FAILURE);
}

struct cell* prim_freecell(struct cell* args)
{
	if(nil == args)
	{
		file_print("Remaining Cells: ", stdout);
		file_print(numerate_number(left_to_take), stdout);
		return nil;
	}
	return make_int(left_to_take);
}

struct cell* string_to_list(char* string)
{
	if(NULL == string) return nil;
	if(0 == string[0]) return nil;
	struct cell* result = make_char(string[0]);
	struct cell* tail = string_to_list(string + 1);
	return make_cons(result, tail);
}

struct cell* prim_string_to_list(struct cell* args)
{
	if(nil == args) return nil;

	if(STRING == args->car->type)
	{
		return string_to_list(args->car->string);
	}
	return nil;
}

int list_length(struct cell* args)
{
	if(nil == args) return 0;
	int size = 0;
	struct cell* i;
	for(i = args->car; nil != i; i = i->cdr)
	{
		size = size + 1;
	}
	return size;
}

char* list_to_string(struct cell* args)
{
	char* string = calloc(list_length(args) + 1, sizeof(char));
	int index = 0;
	struct cell* i;
	for(i = args->car; nil != i; i = i->cdr)
	{
		if(CHAR == i->car->type)
		{
			string[index] = i->car->value;
			index = index + 1;
		}
		else
		{
			file_print("Wrong type recieved\n", stdout);
			exit(EXIT_FAILURE);
		}
	}
	return string;
}

struct cell* prim_list_length(struct cell* args)
{
	if(nil == args) return nil;
	return make_int(list_length(args));
}

struct cell* prim_list_to_string(struct cell* args)
{
	if(nil == args) return nil;
	return make_string(list_to_string(args));
}

struct cell* prim_string_size(struct cell* args)
{
	if(nil == args) return nil;
	if(args->car->type != STRING)
	{
		file_print("Wrong type recieved\n", stderr);
		exit(EXIT_FAILURE);
	}
	return make_int(args->car->size);
}

struct cell* prim_open(struct cell* args, char* mode)
{
	if(nil == args)
	{
		file_print("Did not recieve a file name\n", stderr);
		exit(EXIT_FAILURE);
	}

	if(STRING != args->car->type)
	{
		file_print("File name must be a string\n", stderr);
		exit(EXIT_FAILURE);
	}

	return make_file(open_file(args->car->string, mode));
}


struct cell* prim_open_read(struct cell* args)
{
	return prim_open(args, "r");
}

struct cell* prim_open_write(struct cell* args)
{
	return prim_open(args, "w");
}

struct cell* prim_echo(struct cell* args)
{
	if(nil == args) return nil;
	if(nil == args->car) echo = FALSE;
	if(tee == args->car)
	{
		echo = TRUE;
		return make_string("");
	}
	return args->car;
}

struct cell* prim_read_byte(struct cell* args)
{
	if(nil == args) return make_char(fgetc(input));
	else if(FILE_PORT == args->car->type)
	{
		int c = fgetc(args->car->file);
		if(EOF == c) return make_eof();
		return make_char(c);
	}
	return nil;
}

struct cell* prim_halt(struct cell* args)
{
	/* Cleanup */
	free(args);
	fclose(file_output);

	/* Actual important part */
	exit(EXIT_SUCCESS);
}

struct cell* prim_list(struct cell* args) {return args;}
struct cell* prim_cons(struct cell* args) { return make_cons(args->car, args->cdr->car); }
struct cell* prim_car(struct cell* args) { return args->car->car; }
struct cell* prim_cdr(struct cell* args) { return args->car->cdr; }

void spinup(struct cell* sym, struct cell* prim)
{
	all_symbols = make_cons(sym, all_symbols);
	top_env = extend(top_env, sym, prim);
}

/*** Initialization ***/
void init_sl3()
{
	/* Special symbols */
	nil = make_sym("nil");
	tee = make_sym("#t");
	quote = make_sym("quote");
	s_if = make_sym("if");
	s_cond = make_sym("cond");
	s_lambda = make_sym("lambda");
	s_define = make_sym("define");
	s_setb = make_sym("set!");
	s_begin = make_sym("begin");
	s_let = make_sym("let");
	s_while = make_sym("while");

	/* Globals of interest */
	all_symbols = make_cons(nil, nil);
	top_env = extend(nil, nil, nil);

	/* Add Eval Specials */
	spinup(tee, tee);
	spinup(quote, quote);
	spinup(s_if, s_if);
	spinup(s_cond, s_cond);
	spinup(s_lambda, s_lambda);
	spinup(s_define, s_define);
	spinup(s_setb, s_setb);
	spinup(s_begin, s_begin);
	spinup(s_let, s_let);
	spinup(s_while, s_while);

	/* Add Primitive Specials */
	spinup(make_sym("apply"), make_prim(prim_apply));
	spinup(make_sym("null?"), make_prim(nullp));
	spinup(make_sym("atom?"), make_prim(prim_atom));
	spinup(make_sym("eof-object?"), make_prim(prim_eofp));
	spinup(make_sym("+"), make_prim(prim_sum));
	spinup(make_sym("-"), make_prim(prim_sub));
	spinup(make_sym("*"), make_prim(prim_prod));
	spinup(make_sym("/"), make_prim(prim_div));
	spinup(make_sym("mod"), make_prim(prim_mod));
	spinup(make_sym("and"), make_prim(prim_and));
	spinup(make_sym("or"), make_prim(prim_or));
	spinup(make_sym("not"), make_prim(prim_not));
	spinup(make_sym(">"), make_prim(prim_numgt));
	spinup(make_sym(">="), make_prim(prim_numge));
	spinup(make_sym("="), make_prim(prim_numeq));
	spinup(make_sym("<="), make_prim(prim_numle));
	spinup(make_sym("<"), make_prim(prim_numlt));
	spinup(make_sym("open-input-file"), make_prim(prim_open_read));
	spinup(make_sym("open-output-file"), make_prim(prim_open_write));
	spinup(make_sym("display"), make_prim(prim_display));
	spinup(make_sym("write"), make_prim(prim_write));
	spinup(make_sym("free_mem"), make_prim(prim_freecell));
	spinup(make_sym("get-type"), make_prim(prim_get_type));
	spinup(make_sym("set-type!"), make_prim(prim_set_type));
	spinup(make_sym("list?"), make_prim(prim_listp));
	spinup(make_sym("list"), make_prim(prim_list));
	spinup(make_sym("list-length"), make_prim(prim_list_length));
	spinup(make_sym("list->string"), make_prim(prim_list_to_string));
	spinup(make_sym("string->list"), make_prim(prim_string_to_list));
	spinup(make_sym("string-length"), make_prim(prim_string_size));
	spinup(make_sym("string=?"), make_prim(prim_stringeq));
	spinup(make_sym("cons"), make_prim(prim_cons));
	spinup(make_sym("car"), make_prim(prim_car));
	spinup(make_sym("cdr"), make_prim(prim_cdr));
	spinup(make_sym("echo"), make_prim(prim_echo));
	spinup(make_sym("read-char"), make_prim(prim_read_byte));
	spinup(make_sym("HALT"), make_prim(prim_halt));
}
