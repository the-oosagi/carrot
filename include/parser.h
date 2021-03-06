#ifndef PARSER_H
#define PARSER_H

#include "../include/lexer.h"

#define MAX_NODE_NUM        2048
#define MAX_STR_LITERAL_LEN 512
#define MAX_STATEMENT_NUM   2048
#define MAX_VAR_NAME_LEN    255
#define MAX_VAR_TYPE_LEN    255

typedef struct Node_t Node;

typedef enum {
	N_BLOCK,
	N_BINOP,
	N_UNOP,
	N_FUNC_DEF,
	N_FUNC_CALL,
	N_GET_ITEM,
	N_IF,
	N_ITER,
	N_LITERAL, 
	N_NULL,
	N_RETURN,
	N_STATEMENT,
	N_STATEMENTS,
	N_UNKNOWN,
	N_VAR_DEF,
	N_VAR_ASSIGN,
	N_VAR_ACCESS,
} node_type_t;

typedef enum {
	DT_STR, DT_INT, DT_FLOAT, DT_BOOL, DT_LIST, DT_NULL, DT_UNKNOWN
} data_type_t;


typedef struct Symtable_t {
	char          *key;
	struct Node_t *value;
} Symtable;

typedef struct Node_t {
	node_type_t        type;

	/* value node */
	int                int_val;
	float              float_val;
	char               str_val[MAX_STR_LITERAL_LEN];
	int                bool_val;
	struct Node_t      *obj_val;
	Token              value_token; // shared with variable definition node
	struct Node_t      **list_items; // if a list. TODO: more consistent naming

	/* binary operation node */
	struct Node_t      *left;
	struct Node_t      *right;
	char               op_str[3]; // assuming longest op string of 3

	/* statements node */
	struct Node_t      **statements;

	/* code block node */
	struct Node_t      **block_statements;

	/* variable definition node */
	char               var_name[MAX_VAR_NAME_LEN];
	data_type_t        var_type;
	char               var_type_str[MAX_VAR_TYPE_LEN];
	struct Node_t      *var_node;

	/* function definition node */
	char               func_name[MAX_VAR_NAME_LEN];
	int                is_builtin;
	struct Node_t      **func_params;
	struct Node_t      **func_statements;

	/* item access node */
	struct Node_t      *list_node;
	struct Node_t      *index_node;

	/* if node */
	struct Node_t      **conditions;
	//                 The if_blocks[i] will be executed if
	//                 conditions[i] is true
	struct Node_t      **if_blocks;   
	//                 if none of conditions is true then else_block
	//                 will be executed
	struct Node_t      *else_block;

	/* loop node */
	struct Node_t      *iterable;
	struct Node_t      **loop_statements;
	char               loop_index_var_name[MAX_VAR_NAME_LEN];
	char               loop_iterator_var_name[MAX_VAR_NAME_LEN];
	int                loop_with_index;

	/* function param node */
	char               param_name[MAX_VAR_NAME_LEN];

	/* function call node */
	struct Node_t      **func_args;
	struct Node_t      *callee;
	struct Node_t      *return_value;
} Node;

typedef struct PARSER {
	Token current_token;
	int   i;
	Lexer lexer;
	int   node_cnt;
	Node  *nodes[MAX_NODE_NUM];
} Parser;

Node *init_node();
void free_node(Node *node);
Token parser_consume(Parser *parser);
void parser_free(Parser *parser);
void parser_init(Parser *parser, char *source);
Token parser_lookahed(Parser *parser);
Node *parser_parse(Parser *parser);
Node *parser_parse_arith(Parser *parser);
Node *parser_parse_atom(Parser *parser);
Node *parser_parse_call(Parser *parser);
Node *parser_parse_comp(Parser *parser);
Node *parser_parse_expression(Parser *parser);
Node *parser_parse_factor(Parser *parser);
Node *parser_parse_function_def(Parser *parser, Token id_token);
Node *parser_parse_function_param(Parser *parser);
Node *parser_parse_identifier(Parser *parser);
Node *parser_parse_iter(Parser *parser);
Node *parser_parse_list(Parser *parser);
Node *parser_parse_literal(Parser *parser);
Node *parser_parse_power(Parser *parser);
Node *parser_parse_script(Parser *parser);
Node *parser_parse_statement(Parser *parser);
Node *parser_parse_statements(Parser *parser);
Node *parser_parse_term(Parser *parser);
Node *parser_parse_value(Parser *parser);
Node *parser_parse_variable_def(Parser *parser,
		                    Token id_token,
		                    char *var_type_str, 
				    Token var_value_token,
				    int initialized);

int  carrot_get_args_len(Node *args);
void carrot_get_repr(Node obj, char *out);

void carrot_type_check(tok_kind_t token_kind, char *data_type_str);

#endif
