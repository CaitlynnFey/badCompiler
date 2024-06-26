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
  TokenScopeOpen = 2, // tbd
  TokenScopeClose = 3, // tbd
  TokenIntLit = 4, // data pointer to STRING!! of value; possible child
  TokenPlus = 5, // two children
  TokenMinus = 6, // two children
  TokenMul = 7,
  TokenReturn = 8, // one child only.
  TokenIdent = 9, // data string of ident name
  TokenDiv = 10,
  TokenDeclIdent = 11, // data string of ident name
  TokenFuncCall = 12, // data t_func_call, leaf
  TokenInvalid = 13
} t_tokenType;

const extern char* token_str_lookup[];

typedef struct s_token {
  t_tokenType type;
  struct s_token* children[2];
  void* data; // TokenType defined raw data.
} t_token;

typedef struct s_statementPointer {
  t_token* statement;
  struct s_statementPointer* next;
} t_statement_pointer;

typedef struct s_func_data {
  char* ident;
  t_statement_pointer* statements;
  t_hashtable* identht;
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

void destructor(t_token* t);

void debug_log_token(char* str, t_token*);
t_prog_data* tokenise( char** remaining);
t_token* tryParseExpression(t_token* token, char** remaining, int min_prec);
t_statement_pointer* try_parse_arguments(char** remaining);
#endif
