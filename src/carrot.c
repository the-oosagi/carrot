#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/interpreter.h"
#include "../lib/include/stb_ds.h"

#define MAX_BUFFER_SIZE 1024

char *read_source_file(char *filename) {
	char *source;
	FILE *file = fopen(filename, "r");
	if (!file) {
		printf("Could not open '%s'\n", filename);
		return NULL;
	}

	// calculate source file size
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	source = (char*) malloc(sizeof(char) * (size + 1));
	fread(source, 1, size, file);
	source[size] = '\0';

	fclose(file);

	return source;
}

CarrotObj carrot_func_print(CarrotObj *args) {
	int argc = carrot_get_args_len(args);
	if (argc < 1) {
		printf("ERROR: Function 'print' accepts at least 1 argument");
		exit(1);
	}
	for (int i = 0; i < argc; i++) {
		char s[MAX_STR_LITERAL_LEN];
		carrot_get_repr(args[i], s);
		printf("%s", s);
	}

	return carrot_null();
}

CarrotObj carrot_func_println(CarrotObj *args) {
	arrput(args, carrot_str("\n"));
	return carrot_func_print(args);
}

CarrotObj carrot_func_type(CarrotObj *args) {
	int argc = carrot_get_args_len(args);
	if (argc != 1) {
		printf("ERROR: Function 'type' accepts exactly 1 arguments, but %d are passed.\n", argc);
		exit(1);
	}
	return carrot_str(args[0].var_type_str);
}

void carrot_register_builtin_func(char *name,
		                  CarrotObj (*func)(CarrotObj *args),
		                  Interpreter *interpreter) {
	CarrotObj builtin_func;
	builtin_func.type = N_FUNC_DEF;
	builtin_func.builtin_func = func;
	builtin_func.is_builtin = 1;
	strcpy(builtin_func.func_name, name);
	shput(interpreter->sym_table, name, builtin_func);
}

int main(int argc, char **argv) {
	char *source;
	char *filename = argv[1];

	if (argc != 2) {
		printf("Specify source file");
		exit(1);
	}

	source = read_source_file(filename);
	if (source) {
		Parser parser;
		parser_init(&parser, source);
		CarrotObj n = parser_parse(&parser);
		
		Interpreter interpreter = create_interpreter();

		/* register builtin function */
		carrot_register_builtin_func("print",
				             &carrot_func_print,
					     &interpreter);
		carrot_register_builtin_func("println",
				             &carrot_func_println,
					     &interpreter);
		carrot_register_builtin_func("type",
				             &carrot_func_type,
					     &interpreter);

		interpreter_interpret(&interpreter, &n);
		
		// TODO: free sym_table
		shfree(interpreter.sym_table);

		// TODO: free object arglists, paramlists, etc
		free_node(&n);

		//printf("%d\n", n.statements[0].int_val);
		//printf("===================\n");
		//for (int i = 0; i < parser.lexer.token_cnt; i++) {
		//	printf("%s: ", parser.lexer.tokens[i].text);
		//	printf("%s\n", tok_kind_to_str(parser.lexer.tokens[i].tok_kind));
		//}
		//printf("===================\n");

		parser_free(&parser);
		free(source);

		return 0;
	} else {
		return 1;
	}
}