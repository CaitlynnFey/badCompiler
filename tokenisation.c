#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tokenisation.h"

#define PRECEDENCE_MULTIPLICATION 2
#define PRECEDENCE_ADDITION 1

#define EXT_FAILURE_PARSING 7

const char *token_str_lookup[] = {"invalid", "Name", "assign", "eol", "paren", "scopen", "scope close", "intlit", "expr", "plus", "minus", "mul", "ret", "stmt", "prog", "ident", "div", "invaltoken"};

void destructor(t_token* t) {
	if (t == NULL) 
		return;
	destructor(t->children[0]);
	destructor(t->children[1]);
	free(t->data);
	free(t);
}

void debug_log_token(char* str, t_token* token) {
	printf("%s\t\033[0;32m%p\033[0m, \tTokenType = %s, \tParent: %p \tChildren: %p, %p, \tData: %.8s\n", 
		str, token, token_str_lookup[token->type], token->parent, token->children[0], token->children[1], token->data ? (char*) token->data : "nil");
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

int tokenTypeToPrec(t_tokenType t) {
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

int charToPrec(char c) {
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
	*remaining = whiteSpaceHandler(*remaining);
	size_t intLitPointerOffset = findIntLitPointerOffset(*remaining);
	size_t identPointerOffset = findKeywordPointerOffset(*remaining);

	t_token* returnToken;
	returnToken = malloc(sizeof(t_token));
	returnToken->parent = parent;
	returnToken->children[0] = NULL;
	returnToken->children[1] = NULL;
	
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
		returnToken = tryParseExpression(returnToken, remaining, 0);
		//returnToken->data = NULL;
		//consume close paren
		*remaining = whiteSpaceHandler(*remaining);
		*remaining += 1;
		
		return returnToken;
	}

	fprintf(stderr, "REACHED TOKEN DESTRUCTOR IN TERM PARSER");
	//should never be reached!
	destructor(returnToken);
	return NULL;
}

// based on https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
t_token* tryParseExpression(t_token* parent, char** remaining, int min_prec) {
	printf("parsing expression: %s\n", *remaining);
	
	t_token* returnToken = NULL;
	t_token* left_hand_side = tryParseTerm(NULL, remaining);
	*remaining = whiteSpaceHandler(*remaining);
	
	while(charToPrec(**remaining) && charToPrec(**remaining) >= min_prec) {
		
		printf("found operator %c\n", **remaining);
		
		t_tokenType token_type = charToTokenType(**remaining); //tokenminus
		
		int prec = charToPrec(**remaining); //precaddition
		t_associativity assoc = charToAssoc(**remaining);
		*remaining += 1;
		t_token* tokenA;
		tokenA = malloc(sizeof(t_token));		
		t_token* right_hand_side = tryParseExpression(tokenA, remaining, 
									 assoc == AssociativityLeft ? prec + 1 : prec);
		*remaining = whiteSpaceHandler(*remaining);
		
		right_hand_side->parent = tokenA;
		tokenA->type = token_type;
		tokenA->children[0] = left_hand_side;
		left_hand_side->parent = tokenA;
		tokenA->children[1] = right_hand_side;
		right_hand_side->parent = tokenA;
		tokenA->data = NULL;
		
		if(!returnToken) 
			returnToken = tokenA;
	}


	if(!returnToken) { 
		left_hand_side->parent = parent;
		//debug_log_token("lhs ", left_hand_side);
		printf("created %s token %p in left hand side\n", token_str_lookup[left_hand_side->type], returnToken);		
		return left_hand_side;
	}
	
	if(returnToken) 
		returnToken->parent = parent;
	printf("created %s token %p in right hand side\n", token_str_lookup[returnToken->type], returnToken);	//debug_log_token("mpth ", returnToken);
	return returnToken;
	
	//should never go here!!!!
	fprintf(stderr, "REACHED DESTRUCTOR IN EXPR PARSING");
	destructor(parent);
	return NULL;
}

t_token* tryParseStatement(t_token* parent, char** remaining) {
	printf("parsestatement: %s\n", *remaining);
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

void rec_debug_log_token(char* str, t_token* token) {
	debug_log_token(str, token);
	if(token->children[0]) {
		debug_log_token(str, token->children[0]);	
	}
	if(token->children[1]) {
		debug_log_token(str, token->children[1]);	
	}
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

