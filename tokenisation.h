#ifndef TOKENISATION_H
#define TOKENISATION_H
#include "hashtable.h"
#include <stddef.h>

typedef enum AssociativityTypes {
  AssociativityInvalid,
  AssociativityNone,
  AssociativityRight,
  AssociativityLeft
} t_associativity;

typedef enum TokenTypes {
  TokenDebugInvalid = 0,
  TokenAssign = 1, // data string name of assigned ident
  TokenScope = 2, // data pointer to t_scope_data
  TokenIntLit = 4, // data pointer to STRING!! of value
  TokenPlus = 5, // two children
  TokenMinus = 6, // two children'
  TokenMul = 7,
  TokenReturn = 8, // one child only.
  TokenIdent = 9, // data string of ident name
  TokenDiv = 10, // two children
  TokenDeclIdent = 11, // data string of ident name
  TokenFuncCall = 12, // data t_func_call, leaf
  TokenInvalid = 13
} t_tokenType;

typedef enum basetype {
  integer,
  floating_point
} t_base_type;

const extern char* token_str_lookup[];

typedef struct s_token {
  t_tokenType type;
  struct s_token* children[2];
  void* data; // TokenType defined raw data.
} t_token;

typedef struct s_context {
  t_hashtable* identht;
  struct s_context* caster; //nullable
  struct s_context* shadow; //nullable
} t_context;

typedef struct s_statementPointer {
  t_token* statement;
  struct s_statementPointer* next;
} t_statement_pointer;

typedef struct s_scope_data {
	t_context* ctxt;
	t_statement_pointer* stmts;
} t_scope_data;

typedef struct s_func_data {
  char* ident;
  t_statement_pointer* statements;
  t_context* context;
  size_t args;
} t_func_data;

typedef struct s_func_ptr {
  t_func_data* func;
  struct s_func_ptr* next;
} t_func_ptr;

typedef struct s_prog_data {
  t_func_ptr* funcs;
  t_hashtable* funcht;
} t_prog_data;

typedef struct s_func_call {
  char* ident;
  t_statement_pointer* exprs;
} t_func_call;

typedef struct s_ident {
  char* name;
  t_base_type type;
} t_ident;

void token_destructor(t_token* t);

int ident_destructor(void* ident);

t_context* create_context(t_context* superior_context);
int context_destructor(void* context);

void debug_log_token(char* str, t_token*);
t_prog_data* tokenise(t_prog_data* program, char** remaining);
t_token* tryParseExpression(t_token* token, char** remaining, int min_prec);
t_statement_pointer* try_parse_arguments(char** remaining);
#endif
