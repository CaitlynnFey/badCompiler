#ifndef TOKENISATION_H
#define TOKENISATION_H
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
  TokenIntLit = 4, // data pointer to STRING!! of value; LeafNode
  TokenPlus = 5, // two children
  TokenMinus = 6, // two children
  TokenMul = 7,
  TokenReturn = 8, // one child only.
  TokenProg = 9, // data s_statementPointer; LeafNode  
  TokenIdent = 10, // data string of ident name
  TokenDiv = 11,
  TokenDeclIdent = 12, // data string of ident name
  TokenInvalid = 13
} t_tokenType;

const extern char* token_str_lookup[];

typedef struct s_token {
  t_tokenType type;
  struct s_token* parent; 
  struct s_token* children[2];
  void* data; // TokenType defined raw data.
} t_token;

typedef struct s_statementPointer {
  t_token* statement;
  struct s_statementPointer* next;
} t_statement_pointer;


void destructor(t_token* t);

void debug_log_token(char* str, t_token*);
t_token* tokenise( char** remaining);
t_token* tryParseExpression(t_token* parent, char** remaining, int min_prec);

#endif
