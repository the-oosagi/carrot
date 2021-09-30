#include <stdio.h>
#include "../include/logutils.h"
#include "../include/interpreter.h"
#include "../lib/include/stb_ds.h"

SymTable *CARROT_TRACKING_ARR;

Interpreter create_interpreter() {
	Interpreter interpreter;
	interpreter.parent = NULL;
	interpreter.sym_table = NULL;
	//sh_new_strdup(interpreter.sym_table);
	return interpreter;
}

CarrotObj *interpreter_interpret(Interpreter *interpreter, Node *node) {
	return interpreter_visit(interpreter, node);
}

CarrotObj *interpreter_visit(Interpreter *context, Node *node) {
	switch (node->type) {
		case N_FUNC_CALL:
			return interpreter_visit_func_call(context, node);
		case N_STATEMENTS:
			return interpreter_visit_statements(context, node);
		case N_VAR_DEF:
			return interpreter_visit_var_def(context, node);
		case N_VAR_ACCESS:
			return interpreter_visit_var_access(context, node);
		case N_LITERAL:
			return interpreter_visit_value(context, node);
		// TODO: complete missing cases
		case N_STATEMENT:
		case N_NULL: 
		case N_FUNC_DEF: 
			break;
	}
	printf("%s\n", "ERROR: Unknown node");
	exit(1);
}

CarrotObj *interpreter_visit_func_call(Interpreter *context, Node *node) {
	char *func_name = node->func_name;
	int idx = shgeti(context->sym_table, func_name);
	if (idx == -1) {
		char msg[255];
		sprintf(msg,
		        "Function \"%s\" is undefined. "
			"Make sure you define the function before calling it.",
			func_name);
		carrot_log_error(msg);
		exit(1);
	}

	CarrotObj *func_to_call = shget(context->sym_table, func_name);
	if (func_to_call->is_builtin) {
		CarrotObj **func_args = NULL;
		for (int i = 0; i < arrlen(node->func_args); i++) {
			CarrotObj *itprtd = interpreter_visit(context, node->func_args[i]);
			arrput(func_args, itprtd);
		}
		CarrotObj *res = func_to_call->builtin_func(func_args);

		/* Clean up the evaluated arguments and its representation after
		 * built-in function call */
		//for (int i = 0; i < arrlen(func_args); i++) sdsfree(func_args[i].repr);
		if (func_args != NULL) arrfree(func_args);
		return res;
	} 

	return carrot_null();
}

CarrotObj *interpreter_visit_statements(Interpreter *context, Node *node) {
	int list_item_count = arrlen(node->list_items);

	CarrotObj **list_items = NULL;
	for (int i = 0; i < list_item_count; i++) {
		CarrotObj *item = interpreter_visit(context, node->list_items[i]);
		arrput(list_items, item);
	}

	return carrot_list(list_items);
}

CarrotObj *interpreter_visit_value(Interpreter *context, Node *node) {
	if (node->var_type == DT_STR) {
		return carrot_str(node->value_token.text);
	} else if (node->var_type == DT_INT) {
		return carrot_int(node->int_val);
	} else if (node->var_type == DT_FLOAT) {
		return carrot_float(node->float_val);
	} else if (node->var_type == DT_NULL) {
		return carrot_null();
	} else if (node->var_type == DT_LIST) {
		CarrotObj **list_items = NULL;
		for (int i = 0; i < arrlen(node->list_items); i++) {
			arrput(list_items,
			       interpreter_visit(context, node->list_items[i]));
		}
		return carrot_list(list_items);
	} else {
		printf("The data type for \"%s\" is not supported yet", node->value_token.text);
		exit(1);
	}

}

CarrotObj *interpreter_visit_var_access(Interpreter *context, Node *node) {
	char *var_name = node->var_name;
	CarrotObj *obj = shget(context->sym_table, var_name);

	if (obj == NULL) {
		printf("keys: %s\n", var_name);
		char msg[255];
		sprintf(msg,
		        "You are trying to access variable \"%s\", while it is undefined. "
			"Have you defined it before?",
			var_name);
		carrot_log_error(msg);
		exit(1);
	}


	return shget(context->sym_table, var_name);
}

CarrotObj *interpreter_visit_var_def(Interpreter *context, Node *node) {
	CarrotObj *obj = interpreter_visit(context, node->var_node);
	shput(context->sym_table, node->var_name, obj);
	printf("VISIT VARDEF\n");
	return carrot_null();
}

CarrotObj *carrot_obj_allocate() {
	CarrotObj *obj = calloc(1, sizeof(CarrotObj));

	char *hash = calloc(1, 64);
	sprintf(hash, "%p", (void *) obj);
	obj->hash = hash;
	shput(CARROT_TRACKING_ARR, hash, obj);
	return obj;
}

CarrotObj *carrot_noop() {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_NULL;
	return obj;
}

CarrotObj *carrot_null() {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_NULL;
	obj->type_str = sdsnew("null");
	obj->repr = sdsnew("null");
	return obj;
}

CarrotObj *carrot_int(int int_val) {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_INT;
	obj->type_str = sdsnew("int");
	char repr[255];
	sprintf(repr, "%d", int_val);
	obj->repr = sdsnew(repr);
	return obj;
}

CarrotObj *carrot_list(CarrotObj **list_items) {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_LIST;
	obj->list_items = list_items;
	obj->type_str = sdsnew("list");
	sds repr = sdsnew("[");
	for (int i = 0; i < arrlen(list_items); i++) {
		if (list_items[i]->type == CARROT_STR) {
			repr = sdscat(repr, "\"");
			repr = sdscatsds(repr, list_items[i]->repr);
			repr = sdscat(repr, "\"");
		} else {
			repr = sdscatsds(repr, list_items[i]->repr);
		}

		if (i < arrlen(list_items) - 1)
			repr = sdscat(repr, ", ");
	}
	repr = sdscat(repr, "]");
	obj->repr = repr;
	return obj;
}

CarrotObj *carrot_float(float float_val) {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_FLOAT;
	
	obj->type_str = sdsnew("float");

	char repr[255];
	sprintf(repr, "%f", float_val);
	obj->repr = sdsnew(repr);
	return obj;
}

CarrotObj *carrot_str(char *str_val) {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_STR;
	obj->type_str = sdsnew("str");
	obj->repr = sdsnew(str_val);
	return obj;
}

void carrot_finalize() {
	/* Frees remaining CarrotObj's in heap.
	 * Call this in the very end of main function */
	int len = shlen(CARROT_TRACKING_ARR);
	for (int i = 0; i < len; i++) {
		CarrotObj *obj = CARROT_TRACKING_ARR[i].value;
		shdel(CARROT_TRACKING_ARR, obj->hash);
		carrot_free(obj);
	}

	shfree(CARROT_TRACKING_ARR);
}

void carrot_free(CarrotObj *root) {
	/* It only frees the members of root. If root member is a pointer
	 * to array of allocated objects, it should be freed manually somewhere
	 * else */
	if (arrlen(root->list_items) >= 0) arrfree(root->list_items);
	free(root->hash);
	sdsfree(root->repr);
	sdsfree(root->type_str);
	free(root);
}

void carrot_init() {
	/* Initialize hashtable that tracks CarrotObj's allocated in heap */
	CARROT_TRACKING_ARR = NULL;
	sh_new_strdup(CARROT_TRACKING_ARR);
}

void interpreter_free(Interpreter *interpreter) {
	/* Frees the members of interpreter struct as well as
	 * the content of symbol table. */
	int len = shlen(interpreter->sym_table);
	for (int i = 0; i < len; i++) {
		CarrotObj* obj = interpreter->sym_table[i].value;
		char *hash = obj->hash;
		char *key = interpreter->sym_table[i].key;
		shdel(CARROT_TRACKING_ARR, hash);
		shdel(interpreter->sym_table, key);
		carrot_free(obj);
	}
	shfree(interpreter->sym_table);
}

