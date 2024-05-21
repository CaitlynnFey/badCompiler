#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tokenisation.h"

#define PRECEDENCE_MULTIPLICATION 2
#define PRECEDENCE_ADDITION 1

#define EXT_FAILURE_PARSING 7

#define TOKENISATION_DEBUG

const char *token_str_lookup[] = {"DEBUG INVALID", "Name", "assign", "eol", "paren", "scopen", "scope close", "intlit", "expr", "plus", "minus", "mul", "ret", "stmt", "prog", "ident", "div", "declident", "invaltoken"};

void destructor(t_token* t) {
	if (t == NULL) 
		return;
	destructor(t->children[0]);
	destructor(t->children[1]);
	free(t->data);
	free(t);
}

void debug_log_token(char* str, t_token* token) {
	printf("%s\t\033[0;32m%p\033[0m, TokenType = %s, Parent: %p Children: %p (%s), %p (%s), Data: %.8s\n", 
		str, token, token_str_lookup[token->type], token->parent, token->children[0],
		token->children[0] ? token_str_lookup[token->children[0]->type] : 0, 
		token->children[1], token->children[1] ? token_str_lookup[token->children[1]->type] :
		                            0, token->data ? (char*) token->data : "nil");
}

void rec_debug_log_token(char* str, t_token* token) {
	debug_log_token(str, token);
	if(token->children[0]) {
		debug_log_token(str, token->children[0]);	
	}
	if(token->children[1]) {
		debug_log_token(str, token->children[1]);	
	}
}

size_t findKeywordPointerOffset(char* string) {
	size_t pointerOffset = 0;
	while(isalpha(string[pointerOffset]))
		pointerOffset++;
	return pointerOffset;
}

size_t findIntLitPointerOffset(char* string) {
	size_t pointerOffset = 0;
	while(isdigit(string[pointerOffset]))
		pointerOffset++;
	return pointerOffset;
}

size_t findNonAlNumPointerOffset(char* string) {
	size_t pointerOffset = 0;
	while(isalnum(string[pointerOffset]) || isspace(string[pointerOffset]))
		pointerOffset++;
	return pointerOffset;
}

char* whiteSpaceHandler(char* string) {
	size_t whitespaces = 0;
	while(isspace(string[whitespaces])) 
		whitespaces++;
	return string + whitespaces;
}

t_tokenType charToTokenType(char c) {
	switch (c) {
		case '+':
			return TokenPlus;
		case '-':
			return TokenMinus;
		case '*':
			return TokenMul;
		case '/':
			return TokenDiv;
		case ';':
			return TokenEOL;
		case '{':
			return TokenScopeOpen;
		case '}':
			return TokenScopeClose;
		case '=':
			return TokenAssign;
		default:
			return TokenInvalid;
	}
}

uint8_t tokenTypeToPrec(t_tokenType t) {
	switch (t) {
		case TokenPlus:
		case TokenMinus:
			return PRECEDENCE_ADDITION;
		case TokenMul:
		case TokenDiv:
			return PRECEDENCE_MULTIPLICATION;
		default:
			return 0;
	}
}

uint8_t charToPrec(char c) {
		return tokenTypeToPrec(charToTokenType(c));
}

t_associativity tokenTypeToAssoc(t_tokenType t) {
	switch (t) {
		case TokenMul:
		case TokenPlus:
		case TokenMinus:
		case TokenDiv:
			return AssociativityLeft;
		default:
			return AssociativityInvalid;
	}
}

t_associativity charToAssoc(char c) {
	return tokenTypeToAssoc(charToTokenType(c));
}

t_token* tryParseIdent(t_token* parent, char** remaining) {
	fprintf(stderr, "ident not implemented yet\n");
	return 0;
}

t_token* tryParseTerm(t_token* parent, char** remaining) {
	#ifdef TOKENISATION_DEBUG 
		printf("tryParseTerm, %s", *remaining);
		if(parent != NULL) debug_log_token("parent", parent);
		getchar();
	#endif
	
	*remaining = whiteSpaceHandler(*remaining);
	size_t intLitPointerOffset = findIntLitPointerOffset(*remaining);
	size_t identPointerOffset = findKeywordPointerOffset(*remaining);

	t_token* returnToken;
	returnToken = calloc(1, sizeof(t_token));
	returnToken->parent = parent;
		
	if(intLitPointerOffset) {
		returnToken->type = TokenIntLit;
    char* intLitVal = malloc(intLitPointerOffset + 1);
		strncpy(intLitVal, *remaining, intLitPointerOffset);
		intLitVal[intLitPointerOffset] = '\0';
		returnToken->data = intLitVal;
    *remaining += intLitPointerOffset;
    printf("creating intlit token value %s, pointer %p\n", intLitVal, returnToken);
		return returnToken;

	} else if (identPointerOffset) {
		returnToken->type = TokenIdent;
    returnToken->data = malloc(sizeof(char)* identPointerOffset + 1);
		strncpy(returnToken->data, *remaining, identPointerOffset);
    ((char*)returnToken->data)[identPointerOffset] = '\0';
    *remaining += identPointerOffset;
    printf("creating ident token value %s, pointer %p\n", (char*)returnToken->data, returnToken);		
    return returnToken;
		
	} else if (**remaining == '(') {
		//returnToken->type = TokenParen;
		*remaining += 1;
		free(returnToken);
		t_token* expr = tryParseExpression(NULL, remaining, 0);
		printf("\"Created\" %s token %p as part of a parentheses\n", token_str_lookup[expr->type], expr);
		//returnToken->data = NULL;
		//consume close paren
		*remaining = whiteSpaceHandler(*remaining);
		*remaining += 1;
		
		return expr;
	}

	fprintf(stderr, "REACHED TOKEN DESTRUCTOR IN TERM PARSER");
	//should never be reached!
	destructor(returnToken);
	return NULL;
}

// based on https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
t_token* tryParseExpression(t_token* token, char** remaining, int min_prec) { // mul parent
	#ifdef TOKENISATION_DEBUG 
		printf("tryParseExpression, %u, %s", min_prec, *remaining);
		if(token) debug_log_token("parent", token);
		getchar();
	#endif	

	*remaining = whiteSpaceHandler(*remaining);
	//get lhs
	t_token* lhs = tryParseTerm(NULL, remaining); //should be either a bracketed expression or an int lit
	debug_log_token("lhs", lhs);
	if(!lhs) { //should never happen
		fprintf(stderr, "in tryParseExpression, LHS not found!\n");
		exit(EXIT_FAILURE);
	}

	//prepare return token
	t_token* returnToken = calloc(1, sizeof(t_token));
	returnToken->type = TokenDebugInvalid;

	while(1) {
		*remaining = whiteSpaceHandler(*remaining);
		t_tokenType cur = charToTokenType(**remaining);
		if(!cur || tokenTypeToPrec(cur) == 0 || tokenTypeToPrec(cur) < min_prec) 
			break;
		printf("encountered operator %c\n", **remaining);
		*remaining += 1; //consume operator, will need to be changed given more advanced operators are introduced
		uint8_t prec = tokenTypeToPrec(cur);
		t_associativity assoc = tokenTypeToAssoc(cur);

		t_token* rhs = tryParseExpression(NULL, remaining, prec + (assoc == AssociativityLeft));
		debug_log_token("rhs", rhs);
		
		if(returnToken->type && returnToken->type != TokenInvalid) { //if exists
			if(!tokenTypeToPrec(returnToken->type)) {
				fprintf(stderr, "invalid expression");
				exit(9);
			}
			//paranoia but just in case
			if(returnToken->children[1]) {
				rec_debug_log_token("refit pre", returnToken);
				printf("\n\n");
				t_token* new_token = calloc(1, sizeof(t_token));
				new_token->type = returnToken->type;
				printf("refitting %s token %p with %p\n", token_str_lookup[returnToken->type], returnToken, new_token);
				new_token->children[0] = returnToken->children[1];
				new_token->children[1] = rhs;
				returnToken->children[1] = new_token;
				rec_debug_log_token("refit log", returnToken);
			} else {
				returnToken->children[1] = rhs;
			}
		} else {
			printf("creating %s token %p\n", token_str_lookup[cur], returnToken);
			returnToken->type = cur;
			returnToken->children[0] = lhs;
			returnToken->children[1] = rhs;
		}
	}

	if(returnToken->type == TokenInvalid || returnToken->type == 0) {
		free(returnToken);
		printf("invalid return token; returning %s token %p from tryParseExpr\n", token_str_lookup[lhs->type], lhs);
		return lhs;
	}

	return returnToken;
}

t_token* tryParseStatement(t_token* parent, char** remaining) {
	#ifdef TOKENISATION_DEBUG 
		printf("tryParseStatement, %s", *remaining);
		debug_log_token("parent", parent);
		getchar();
	#endif	
	*remaining = whiteSpaceHandler(*remaining);
	size_t pointerOffset = findKeywordPointerOffset(*remaining);

	if(!pointerOffset) 
		return NULL;
	
	t_token* returnToken;
	returnToken = malloc(sizeof(t_token));	
	returnToken->children[0] = NULL;
	returnToken->children[1] = NULL;
	returnToken->parent = parent;	
	if(!strncmp("let", *remaining, pointerOffset)) {
		printf("parsed let\n");
		*remaining += pointerOffset;
		returnToken->type = TokenDeclIdent;
		*remaining = whiteSpaceHandler(*remaining);
		size_t keywordPointerOffset = findKeywordPointerOffset(*remaining);
		if(!keywordPointerOffset) {
			fprintf(stderr, "let not followed by ident!\n");
			exit(EXT_FAILURE_PARSING);
		}
		char* ident = malloc(sizeof(char) * keywordPointerOffset + 1);
		ident[keywordPointerOffset] = '\0';
		strncpy(ident, *remaining, keywordPointerOffset);
		returnToken->data = ident;
		*remaining += keywordPointerOffset;
		*remaining = whiteSpaceHandler(*remaining);
		printf("parsed ident, ident: %s\n", (char*) returnToken->data);

		if(**remaining == '=') {
			*remaining += 1;
			*remaining = whiteSpaceHandler(*remaining);
			t_token* assignToken = malloc(sizeof(t_token));
			assignToken->type = TokenAssign;
			assignToken->parent = returnToken;
			assignToken->children[0] = tryParseExpression(assignToken, remaining, 0);
			assignToken->children[1] = NULL;
			if(!assignToken->children[0]) {
				fprintf(stderr, "expected expression after '='!\n");
				exit(EXT_FAILURE_PARSING);
			}
			printf("parsed =\n");
			assignToken->data = ident;
			returnToken->children[0] = assignToken;
		}
		*remaining = whiteSpaceHandler(*remaining);
		if(**remaining != ';') {
			fprintf(stderr, "error parsing let, no semicolon\n");
			exit(EXT_FAILURE_PARSING);
		}
		*remaining += 1;
		debug_log_token("let", returnToken);
		if(returnToken->children[0])
			debug_log_token("ltcld", returnToken->children[0]);
		return returnToken;
	} else if (!strncmp("return", *remaining, pointerOffset)) {
		returnToken->type = TokenReturn;
		*remaining += pointerOffset;
		returnToken->children[0] = 
			tryParseExpression(returnToken, remaining, 0);
		if(!returnToken->children[0]) {
			fprintf(stderr, "failure parsing return! No Expr!\n");
			exit(EXT_FAILURE_PARSING);
		}
		*remaining = whiteSpaceHandler(*remaining);
		if(**remaining != ';') {
			fprintf(stderr, "failure parsing return! no semicolon!\n");
			exit(EXT_FAILURE_PARSING);
		}
		*remaining += 1;
		return returnToken;
	}
	destructor(returnToken);
	return NULL;
}

t_token* tokenise(char** remaining) {
	t_token* returnToken;
	returnToken = malloc(sizeof(t_token));
	returnToken->type = TokenProg;
	returnToken->children[0] = NULL;
	returnToken->children[1] = NULL;
	returnToken->parent = NULL;

	t_statement_pointer* prev = NULL;
	for(t_token* t; t; t = tryParseStatement(returnToken, remaining)) {
		//rec_debug_log_token("tkrc", t);
		printf("pointer: %p\n", t);
		t_statement_pointer* p = malloc(sizeof(t_statement_pointer));
		p->next = NULL;
		p->statement = t;
		if (!p->statement->type) {
			break;
		}
		if(prev) {
		prev->next = p;
		} else {
			returnToken->data = p;
		}
		prev = p;
	}
	return returnToken;
}

