#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

//If we are compiling on Windows compile these functions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

//Fake readline function
char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

//Fake add_history function
void add_history(char* unused) {}

//Otherwise include the editline headers
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

//Create Enumeration of Possible lval Types
enum {
	LVAL_NUM,
	LVAL_ERR
};

//Create Enumeration of Possible Error Types
enum {
	LERR_DIV_ZERO,
	LERR_BAD_OP,
	LERR_BAD_NUM,
};

//Declare new lval struct
typedef struct {
	int type;
	long num;
	int err;
} lval;

//Create a new number type lval
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

//Create a new error type lval
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

//Print an "lval"
void lval_print(lval v) {
	switch (v.type) {
		//If the type is number print it
		case LVAL_NUM: printf("%li\n", v.num); break;

		//If the type is error
		case LVAL_ERR:
			switch (v.err) {
				case LERR_DIV_ZERO: printf("Error: Division by zero!\n"); break;
				case LERR_BAD_OP: printf("Error: Invalid Operator!\n"); break;
				case LERR_BAD_NUM: printf("Error: Invalid Numer!\n"); break;
			}
	}
}

int number_of_nodes(mpc_ast_t* t) {
	if (t->children_num == 0) {
		return 1;
	}
	if (t->children_num >= 1) {
		int total = 1;
		for (int i=0; i< t->children_num; i++) {
			total = total + number_of_nodes(t->children[i]);
		}
		return total;
	}
	return 0;
}

lval eval_op(lval x, char* op, lval y) {

	if (x.type == LVAL_ERR) {return x; }
	if (y.type == LVAL_ERR) {return y; }

	if (strcmp(op, "+") == 0) 		{return lval_num(x.num + y.num); }
	else if (strcmp(op, "-") == 0)  {return lval_num(x.num - y.num); }
	else if (strcmp(op, "*") == 0)  {return lval_num(x.num * y.num); }
	else /*(strcmp(op, "/") == 0)*/ {
		
		//if second operand is zero return error
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num / y.num);
	}
}

lval eval(mpc_ast_t* t) {
	
	//if tagged as a number
	if (strstr(t->tag, "number")) {
		//Check if there is an error in conversion
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE 
			? lval_num(x) 
			: lval_err (LERR_BAD_NUM);
	}

	//The operator is always second child
	char *op = t->children[1]->contents;

	//We store the third child in x
	lval x = eval(t->children[2]);

	//Iterate the remaining children and combining
	int i=3;
	while (strstr(t->children[i]->tag, "expr"))	{
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}

int main(int argc, char** argv) {

	//Create some Parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	//Define them with the following language
	mpca_lang(MPCA_LANG_DEFAULT,
		"															\
			number		: /-?[0-9]+/ ;								\
			operator	: '+' | '-' | '*' | '/' ;					\
			expr		: <number> | '(' <operator> <expr>+ ')' ;	\
			lispy		: /^/ <operator> <expr>+ /$/ ;				\
		",
		Number, Operator, Expr, Lispy);

	//Print Version and Exit Information
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	//In a never ending loop
	while (1) {
		//Output our prompt and get input
		char* input = readline("lispy> ");
		
		//Add input to history
		add_history(input);

		//Attempt to parse the user input
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			//printf("hello");
			lval_print(result);
			mpc_ast_delete(r.output);
		} else {
			//Otherwise print the error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}

	//Undefine and delete our Parsers
	mpc_cleanup(4, Number, Operator, Expr, Lispy);

	return 0;
}