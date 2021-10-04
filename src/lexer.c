#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"
#include "../include/logutils.h"


int is_keyword(char *s) {
	int n_keywords = 4;
	char keywords[4][20] = {
		#include "../spec/keywords"
	};

	int found = 0;
	for (int i = 0; i < n_keywords; i++) {
		if (strcmp(s, keywords[i]) == 0) {
			found = 1;
			break;
		}
	}
	return found;
}

int is_escape(char* s) {
	const int n_escape = 6;
	char escapes[6][3] = {
		#include "../spec/escapes"
	};

	for (int i = 0; i < n_escape; i++) {
		if (strcmp(s, escapes[i]) == 0) {
			return 1;
		}
	}

	return 0;
}

/* I think make_escape is not used by anywhere
   except this translation unit so i marked this
   function as static

   By now, escape character only support single char
*/
static char make_escape(char* s) {
	switch(s[1]) {
		case 'r' : return '\r';
		case 'n' : return '\n';
		case '\\': return '\\';
		case '\'': return '\'';
		case '"' : return '"';
		case 't' : return '\t';
	}
	return 0; // is not found
}

char *tok_kind_to_str(tok_kind_t kind) {
	switch (kind) {
		case T_INT: return "T_INT";
		case T_FLOAT: return "T_FLOAT";
		case T_STR: return "T_STR";
		case T_ID: return "T_ID";
		case T_KEYWORD: return "T_KEYWORD";
		case T_PLUS: return "T_PLUS";
		case T_MINUS: return "T_MINUS";
		case T_EQUAL: return "T_EQUAL";
		case T_COLON: return "T_COLON";
		case T_RARROW: return "T_RARROW";
		case T_EOF: return "T_EOF";
		case T_LPAREN: return "T_LPAREN";
		case T_RPAREN: return "T_RPAREN";
		case T_LBRACKET: return "T_LBRACKET";
		case T_RBRACKET: return "T_RBRACKET";
		case T_MULT: return "T_MULT";
		case T_DIV: return "T_DIV";
		case T_COMMA: return "T_COMMA";
		case T_UNKNOWN: return "T_UNKNOWN";
	} 
	return "T_UNKNOWN";
}

Token create_token(tok_kind_t tok_kind, char *text) {
	Token t;
	t.tok_kind = tok_kind;
	strncpy(t.text, text, MAX_TOKEN_TEXT_LEN);
	return t;
}

void make_identifier(Lexer *lexer) {
	int i = 0;
	char s[MAX_TOKEN_TEXT_LEN] = "";
	while (isalpha(lexer->c) || lexer->c == '_') {
		s[i++] = lexer->c;
		lexer_next(lexer);
	}
	s[i] = '\0';

	if (is_keyword(s)) {
		lexer_add_token(lexer, create_token(T_KEYWORD, s));
	} else {
		lexer_add_token(lexer, create_token(T_ID, s));
	}
}

void make_number(Lexer *lexer) {
	int i = 0;
	int num_dot = 0;
	char s[MAX_TOKEN_TEXT_LEN] = "";
	while (isdigit(lexer->c) || lexer->c == '.') {
		if (lexer->c == '.') num_dot++;
		s[i++] = lexer->c;
		lexer_next(lexer);
	}
	s[i] = '\0';

	if (num_dot > 1) {
		char msg[255];
		snprintf(msg, 255, "I saw an invalid number format in '%s' at line %d. "
			 "You typed: %s\n",
			 "foo",
			 lexer->line_num,
			 s);
		carrot_log_error(msg, "idklol", -1);
		exit(1);
	}

	if (num_dot == 1) {
		lexer_add_token(lexer, create_token(T_FLOAT, s));
	} else {
		lexer_add_token(lexer, create_token(T_INT, s));
	}
}

void make_single_char_token(Lexer *lexer, tok_kind_t kind, char *text) {
	lexer_add_token(lexer, create_token(kind, text));
	lexer_next(lexer);
}

void make_two_chars_token(Lexer *lexer, tok_kind_t kind, char *text) {
	lexer_add_token(lexer, create_token(kind, text));
	lexer_next(lexer);
	lexer_next(lexer);
}

void make_string(Lexer *lexer) {
	lexer_next(lexer);
	char s[MAX_TOKEN_TEXT_LEN] = "";
	int i = 0;
	while (lexer->c != '"') {
		if (lexer->c == '\\') {
			/* Scan for escape characters */
			/* Case that should be taken care of:
			   lexer->source[++lexer->idx] pasts the array
			*/
			char e[3] = {lexer->c, lexer->source[++lexer->idx], 0};
			if (is_escape(e)) {
				s[i++] = make_escape(e);
				// lexer_next(lexer);
			}
			else {
				fprintf(stderr, "Illegal escape character %s at line %d.\n", e, lexer->line_num);
				exit(1);
			}
		} else {
			s[i++] = lexer->c;
		}
		lexer_next(lexer);
	}
	
	// found enclosing delimiter
	lexer_next(lexer);
	s[i] = '\0';

	lexer_add_token(lexer, create_token(T_STR, s));
}

void lexer_add_token(Lexer *lexer, Token t) {
	lexer->tokens[lexer->token_cnt++] = t;
}

void lexer_init(Lexer *lexer, char *source) {
	lexer->idx = 0;
	lexer->line_num = 1;
	lexer->source = source;
	lexer->c = lexer->source[0];
	lexer->token_cnt = 0;
}

void lexer_next(Lexer *lexer) {
	if (lexer->c != '\0')
		lexer->c = lexer->source[++lexer->idx];
}

void lexer_skip_comment(Lexer *lexer) {
	if (lexer->source[lexer->idx+1] == '-') {
		while (lexer->c != '\n') {
			lexer_next(lexer);
		}
		lexer->line_num++;
	}
}

void lexer_skip_whitespace(Lexer *lexer) {
	while (isspace(lexer->c)) {
		if (lexer->c == '\n') lexer->line_num++;
		lexer_next(lexer);
	}
}

/*===========================================================================
 *Lexical Analysis
 *===========================================================================*/
void lexer_lex(Lexer *lexer) {
	while (lexer->c != '\0') {
		lexer_skip_whitespace(lexer);

		if (isalpha(lexer->c) || lexer->c == '_') {
			make_identifier(lexer);
		} else if (isdigit(lexer->c)) {
			make_number(lexer);
		} else if (lexer->c == '"') {
			make_string(lexer);
		} else if (lexer->c == '=') {
			make_single_char_token(lexer, T_EQUAL, "=");
		} else if (lexer->c == '(') {
			make_single_char_token(lexer, T_LPAREN, "(");
		} else if (lexer->c == ')') {
			make_single_char_token(lexer, T_RPAREN, ")");
		} else if (lexer->c == '[') {
			make_single_char_token(lexer, T_LBRACKET, "[");
		} else if (lexer->c == ']') {
			make_single_char_token(lexer, T_RBRACKET, "]");
		} else if (lexer->c == ',') {
			make_single_char_token(lexer, T_COMMA, ",");
		} else if (lexer->c == '+') {
			make_single_char_token(lexer, T_PLUS, "+");
		} else if (lexer->c == ':') {
			make_single_char_token(lexer, T_COLON, ":");
		} else if (lexer->c == '-') {
			if (lexer->source[lexer->idx+1] == '-') {
				lexer_skip_comment(lexer);
			} else if (lexer->source[lexer->idx+1] == '>') {
				make_two_chars_token(lexer, T_RARROW, "->");
			} else {
				make_single_char_token(lexer, T_MINUS, "-");
			}
		} else {
			lexer_next(lexer);
			//char msg[100];
			//sprintf(msg,"Unexpected character: \"%c\"\n", lexer->c);
			//carrot_log_error(msg, "idklol", lexer->line_num);
			//exit(1);
		}
	}
	lexer_add_token(lexer, create_token(T_EOF, "EOF"));
}
