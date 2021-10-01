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
		case N_FUNC_DEF: 
			return interpreter_visit_func_def(context, node);
		case N_RETURN: 
			return interpreter_visit_return(context, node);
		// TODO: complete missing cases
		case N_STATEMENT:
		case N_NULL: 
		case N_UNKNOWN: 
			break;
	}
	printf("%s\n", "ERROR: Unknown node");
	exit(1);
}

CarrotObj *interpreter_visit_func_call(Interpreter *context, Node *node) {
	char *func_name = node->func_name;
	CarrotObj *func_to_call = carrot_get_var(func_name, context);
	if (func_to_call == NULL) {
		char msg[255];
		sprintf(msg,
		        "Function \"%s\" is undefined. "
			"Make sure you define the function before calling it.",
			func_name);
		carrot_log_error(msg, "idklol", -1);
		exit(1);
	}

	if (func_to_call->is_builtin) {
		/* Case 1: the function being called is a builtin function */
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
	} else {
		/* Case 2: the function being called is made inside carrot script */
		// -------
		//	Populate local variables within the function based on 
		//	argument names
		CarrotObj *return_value = NULL;
		Interpreter local_interpreter = create_interpreter();
		local_interpreter.parent = context;
		for (int i = 0; i < arrlen(func_to_call->func_arg_names); i++) {
			char *argname = func_to_call->func_arg_names[i];
			CarrotObj *argval = interpreter_visit(
				&local_interpreter,
				node->func_args[i]
			);
			shput(local_interpreter.sym_table, argname, argval);
		}
		//      Evaluate the function body (a list of statements)
		//
		for (int i = 0; i < arrlen(func_to_call->func_statements); i++) {
			interpreter_visit(&local_interpreter,
			                  func_to_call->func_statements[i]);
			
			// TODO: check if func_to_call->func_statements[i] node is
			// a N_RETURN node, and return accordingly
			Node *stmt = func_to_call->func_statements[i];
			if (stmt->type == N_RETURN) {
				return_value = interpreter_visit(&local_interpreter,
						                 stmt);
				break;
			}
		}

		//      End the local variable lifetime, except if it refers to
		//      the return value object
		int len = shlen(local_interpreter.sym_table);
		for (int i = 0; i < len; i++) {
			/* if return value obj also belongs to local sym_table,
			 * detach it first so it is not wiped and persists after
			 * leaving this function. It will still be tracked by
			 * the global tracker */
			if (local_interpreter.sym_table[i].value == return_value) {
				shdel(local_interpreter.sym_table,
				      local_interpreter.sym_table[i].key);
			}
		}
		interpreter_free(&local_interpreter);
		if (return_value==NULL) return carrot_null();
		return return_value;
	}
}

CarrotObj *interpreter_visit_func_def(Interpreter *context, Node *node) {
	CarrotObj *function = carrot_obj_allocate();
	function->func_statements = node->func_statements;
	strcpy(function->func_name, node->func_name);
	for (int i = 0; i < arrlen(node->func_params); i++) {
		//printf("%s\n", node->func_params[i]->param_name);
		arrput(function->func_arg_names, node->func_params[i]->param_name);
	}
	shput(context->sym_table, node->func_name, function);
	return carrot_null();
}

CarrotObj *interpreter_visit_return(Interpreter *context, Node *node) {
	return interpreter_visit(context, node->return_value);
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
	CarrotObj *obj = carrot_get_var(var_name, context);

	if (obj == NULL) {
		char msg[255];
		sprintf(msg,
		        "You are trying to access variable \"%s\", while it is undefined. "
			"Have you defined it before?",
			var_name);
		carrot_log_error(msg, "idklol", -1);
		exit(1);
	}

	return obj;
}

CarrotObj *interpreter_visit_var_def(Interpreter *context, Node *node) {
	CarrotObj *obj = interpreter_visit(context, node->var_node);
	shput(context->sym_table, node->var_name, obj);
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

CarrotObj *carrot_get_var(char *var_name, Interpreter *context) {
	/* look up the variable based on name. If it is not found 
	 * in the context's sym_table, then recursicely look up
	 * on context's parent interpreter */
	CarrotObj *obj = shget(context->sym_table, var_name);
	if (obj != NULL)
		return obj;

	if (obj == NULL && context->parent != NULL) {
		return carrot_get_var(var_name, context->parent);
	}

	return NULL;
}

CarrotObj *carrot_int(int int_val) {
	CarrotObj *obj = carrot_obj_allocate();
	obj->type = CARROT_INT;
	obj->type_str = sdsnew("int");
	obj->repr = sdscatprintf(sdsempty(), "%d", int_val);
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
	obj->repr = sdscatprintf(sdsempty(), "%f", float_val);
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
	if (arrlen(root->func_arg_names) >= 0) arrfree(root->func_arg_names);
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

