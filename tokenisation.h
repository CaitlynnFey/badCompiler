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
  TokenDeclIdent = 17,
  TokenName = 1,
  TokenAssign = 2,
  TokenEOL = 3,
  TokenParen = 4,
  TokenScopeOpen = 5,
  TokenScopeClose = 6,
  TokenIntLit = 7, //data pointer to STRING!! of value; LeafNode
  TokenExpr = 8,
  TokenPlus = 9,
  TokenMinus = 10,
  TokenMul = 11,
  TokenReturn = 12, //one child only.
  TokenStmt = 13,
  TokenProg = 14, //data s_statementPointer; LeafNode BUT parent
  TokenIdent = 15,
  TokenDiv = 16,
  TokenInvalid = 18
} t_tokenType;


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
