#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tokenisation.h"
#include "hashtable.h"

#define PRECEDENCE_MULTIPLICATION 2
#define PRECEDENCE_ADDITION 1

#define EXT_FAILURE_PARSING 7

#define WHITESPACE() *remaining = whiteSpaceHandler(*remaining)

// #define TOKENISATION_DEBUG

const char *token_str_lookup[] = {"DEBUG INVALID", "assign", "scopen", "scope close", "intlit", "plus", "minus", "mul", "ret", "prog", "ident", "div", "declident", "declfunc", "funccall", "invaltoken"};

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

void expect_consume_char(char** remaining, char c) {
	if(**remaining != c) {
		fprintf(stderr, "Failed expect_consume_char with string \"%s\", expecting char '%c'\n", *remaining, c);
		exit(9);
	}
	*remaining += 1;
}

uint_least8_t try_consume_char(char** remaining, char c) {
	uint_least8_t ret = **remaining == c;
	*remaining += 1;
	return ret;
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
	char flags; // LSB line comment, next one up block comment
	while(1) {
	if(*(string + whitespaces + 1) == '\0')
			return string + whitespaces + 1;
		
		if(flags) {
			if((flags & 1) && *(string + whitespaces) == '\n') {
				whitespaces++;
				flags ^= 1;
				continue;
			}
			
			if((flags & 2) && *(string + whitespaces) == '*' && * (string + whitespaces + 1) == '/') {
				whitespaces += 2;
				flags ^= 2;
				continue;
			}
			whitespaces++;
			continue;
		}
		
		if(*(string + whitespaces) == '/') {
			if(*(string + whitespaces + 1) == '/') {
				whitespaces += 2;
				flags ^= 1;
				continue;
			}
			
			if(*(string + whitespaces + 1) == '*') {
				whitespaces += 2;
				flags ^= 2;
				continue;
			}
		}

		if(!isspace(*(string + whitespaces))) 
			break;	
		whitespaces++;
	}
	return string + whitespaces;
}

size_t getFunctionNameOffset(char* string) {
	char* paren = strchr(string, '(');
	if(!paren)
		return 0; 
	size_t offset = 1;
	uint_fast8_t flag;
	while(1) {
		if(flag) {
			if(*(paren - offset) == '*' && *(paren - offset - 1) == '/') {
				flag = 0;
				offset += 2;
				continue;
			}
			offset++;
			continue;
		}
		if(isspace(*(paren - offset))) {
			offset++;
			continue;
		}
		if(*(paren - offset) == '/' && *(paren - offset - 1) == '*') {	
			flag = 1;
			offset += 2;
			continue;
		}
		break;	
	}
	return paren - string - offset;
}


void expect_consume_chars(char** remaining, const char* str) {
	for(size_t i = 0; i < strlen(str); i++) {
		WHITESPACE();
		expect_consume_char(remaining, str[i]);
	}
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
	
	WHITESPACE();
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
    #ifdef TOKENISATION_DEBUG
    	printf("creating intlit token value %s, pointer %p\n", intLitVal, returnToken);
    #endif
		return returnToken;

	} else if (identPointerOffset) {
		returnToken->type = TokenIdent;
    returnToken->data = malloc(sizeof(char)* identPointerOffset + 1);
		strncpy(returnToken->data, *remaining, identPointerOffset);
    ((char*)returnToken->data)[identPointerOffset] = '\0';
    *remaining += identPointerOffset;
    #ifdef TOKENISATION_DEBUG
    	printf("creating ident token value %s, pointer %p\n", (char*)returnToken->data, returnToken);		
    #endif
    return returnToken;
		
	} else if (**remaining == '(') {
		*remaining += 1;
		free(returnToken);
		t_token* expr = tryParseExpression(NULL, remaining, 0);
		#ifdef TOKENISATION_DEBUG
			printf("\"Created\" %s token %p as part of a parentheses\n", token_str_lookup[expr->type], expr);
		#endif
		//consume close paren
		WHITESPACE();
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

	WHITESPACE();
	//get lhs
	t_token* lhs = tryParseTerm(NULL, remaining); //should be either a bracketed expression or an int lit
	
	#ifdef TOKENISATION_DEBUG
		debug_log_token("lhs", lhs);
	#endif
	
	if(!lhs) { //should never happen
		fprintf(stderr, "in tryParseExpression, LHS not found!\n");
		exit(EXIT_FAILURE);
	}

	//prepare return token
	t_token* returnToken = calloc(1, sizeof(t_token));
	returnToken->type = TokenDebugInvalid;

	while(1) {
		WHITESPACE();
		t_tokenType cur = charToTokenType(**remaining);
		if(!cur || tokenTypeToPrec(cur) == 0 || tokenTypeToPrec(cur) < min_prec) 
			break;
		
		#ifdef TOKENISATION_DEBUG
			printf("encountered operator %c\n", **remaining); //NOLINT
		#endif 
		
		*remaining += 1; //consume operator, will need to be changed given more advanced operators are introduced
		uint8_t prec = tokenTypeToPrec(cur);
		t_associativity assoc = tokenTypeToAssoc(cur);

		t_token* rhs = tryParseExpression(NULL, remaining, prec + (assoc == AssociativityLeft));

		#ifdef TOKENISATION_DEBUG
			debug_log_token("rhs", rhs);
		#endif
		
		if(returnToken->type && returnToken->type != TokenInvalid) { //if exists
			if(!tokenTypeToPrec(returnToken->type)) {
				fprintf(stderr, "invalid expression");
				exit(9);
			}
			//paranoia but just in case
			if(returnToken->children[1]) {
				#ifdef TOKENISATION_DEBUG
					rec_debug_log_token("refit pre", returnToken);
					printf("\n\n");
				#endif
				
				t_token* new_token = calloc(1, sizeof(t_token));
				new_token->type = returnToken->type;

				#ifdef TOKENISATION_DEBUG
					printf("refitting %s token %p with %p\n", token_str_lookup[returnToken->type], returnToken, new_token);
				#endif
				
				new_token->children[0] = returnToken->children[1];
				new_token->children[1] = rhs;
				returnToken->children[1] = new_token;

				#ifdef TOKENISATION_DEBUG
					rec_debug_log_token("refit log", returnToken);
				#endif
				
			} else {
				returnToken->children[1] = rhs;
			}
		} else {
			#ifdef TOKENISATION_DEBUG
				printf("creating %s token %p\n", token_str_lookup[cur], returnToken);
			#endif
			
			returnToken->type = cur;
			returnToken->children[0] = lhs;
			returnToken->children[1] = rhs;
		}
	}

	if(returnToken->type == TokenInvalid || returnToken->type == 0) {
		free(returnToken);

		#ifdef  TOKENISATION_DEBUG
			printf("invalid return token; returning %s token %p from tryParseExpr\n", token_str_lookup[lhs->type], lhs);
		#endif
		
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
	WHITESPACE();
	size_t pointerOffset = findKeywordPointerOffset(*remaining);

	if(!pointerOffset) 
		return NULL;
	
	t_token* returnToken;
	returnToken = malloc(sizeof(t_token));	
	returnToken->children[0] = NULL;
	returnToken->children[1] = NULL;
	returnToken->parent = parent;	
	
	if(!strncmp("let", *remaining, pointerOffset)) {
		#ifdef TOKENISATION_DEBUG
			printf("parsed let\n");
		#endif
		
		*remaining += pointerOffset;
		returnToken->type = TokenDeclIdent;
		WHITESPACE();
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
		WHITESPACE();
		
		#ifdef TOKENISATION_DEBUG
			printf("parsed ident, ident: %s\n", (char*) returnToken->data);
		#endif
		
		if(**remaining == '=') {
			*remaining += 1;
			WHITESPACE();
			t_token* assignToken = malloc(sizeof(t_token));
			assignToken->type = TokenAssign;
			assignToken->parent = returnToken;
			assignToken->children[0] = tryParseExpression(assignToken, remaining, 0);
			assignToken->children[1] = NULL;
			if(!assignToken->children[0]) {
				fprintf(stderr, "expected expression after '='!\n");
				exit(EXT_FAILURE_PARSING);
			}
			#ifdef TOKENISATION_DEBUG
				printf("parsed =\n");
			#endif
			assignToken->data = ident;
			returnToken->children[0] = assignToken;
		}
		WHITESPACE();
		if(**remaining != ';') {
			fprintf(stderr, "error parsing let, no semicolon\n");
			exit(EXT_FAILURE_PARSING);
		}
		
		*remaining += 1;
		
		#ifdef TOKENISATION_DEBUG
			debug_log_token("let", returnToken);
			if(returnToken->children[0])
				debug_log_token("ltcld", returnToken->children[0]);
		#endif
		
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
		WHITESPACE();
		if(**remaining != ';') {
			fprintf(stderr, "failure parsing return! no semicolon!\n");
			exit(EXT_FAILURE_PARSING);
		}
		*remaining += 1;
		return returnToken;
	}

	returnToken->type = TokenIdent;
	returnToken->data = calloc(pointerOffset + 1, 1);
	memcpy(returnToken->data, *remaining, pointerOffset);
	*remaining += pointerOffset;
	WHITESPACE();

	if(**remaining == '=') {
		*remaining += 1;
		returnToken->type = TokenAssign;
		returnToken->children[0] = tryParseExpression(returnToken, remaining, 0);
	}

	WHITESPACE();
	if(**remaining != ';') {
		fprintf(stderr, "failure parsing statement! no semicolon!\n");
		exit(EXT_FAILURE_PARSING);
	}
	*remaining += 1;
	return returnToken;
}

t_hashtable* tryParseIdentList(char** remaining, t_hashtable* ht) {
	size_t i = 0;
	do {
		WHITESPACE();
		size_t keywordoffset = findKeywordPointerOffset(*remaining);
		char* key = calloc(keywordoffset + 2, sizeof(char));
		memcpy(key, *remaining,keywordoffset + 1);
		t_htentry* entry = calloc(1, sizeof (t_htentry));
		entry->key = key;
		entry->value = calloc(1, sizeof(size_t));
		*((size_t*) entry->value) = i;
		
		ht = hashtable_put(ht, entry);
		*remaining += keywordoffset;
		WHITESPACE();
	} while (try_consume_char(remaining, ','));
	*remaining -= 1;
	return ht;
}

t_token* tryParseFunction(char** remaining) {
	WHITESPACE();
	size_t fn_idnt_offset = getFunctionNameOffset(*remaining);
	if(fn_idnt_offset == 0)
		return NULL;
	char* fn_idnt = calloc(fn_idnt_offset + 2, sizeof(char));
	memcpy(fn_idnt, *remaining, fn_idnt_offset + 1);
	*remaining += fn_idnt_offset + 1;
	WHITESPACE();
	t_func_data* data = calloc(1, sizeof(t_func_data));
	data->ident = fn_idnt;
	
	expect_consume_char(remaining, '(');
	data->identht = tryParseIdentList(remaining, hashtable_create());
	WHITESPACE();
	expect_consume_char(remaining, ')');
	WHITESPACE();	
	
	expect_consume_char(remaining, '{');
	t_token* ret = calloc(1, sizeof(t_token));
	ret->type = TokenDeclFunc;
	ret->data = data;

	t_statement_pointer* prev = NULL;	
	for(t_token* t = tryParseStatement(ret, remaining); t; t = tryParseStatement(ret, remaining)) {
		if(t->type < TokenDebugInvalid || t->type > TokenInvalid)
			break;
		t_statement_pointer* p = calloc(1, sizeof(t_statement_pointer));
		p->statement = t;
		if(prev) 
			prev->next = p;
		else 
			data->statements = p;
		prev = p;
	}
	WHITESPACE();
	expect_consume_char(remaining, '}');	
	#ifdef TOKENISATION_DEBUG
		printf("created function token %#zx ident %s\n", (size_t)ret, ((t_func_data*)ret->data)->ident);
	#endif
	return ret;
}

t_token* tokenise(char** remaining) {
	t_token* returnToken;
	returnToken = malloc(sizeof(t_token));
	returnToken->type = TokenProg;
	returnToken->children[0] = NULL;
	returnToken->children[1] = NULL;
	returnToken->parent = NULL;

	t_hashtable* ht = hashtable_create();
	for(t_token* t = tryParseFunction(remaining); t; t = tryParseFunction(remaining)) {
		#ifdef TOKENISATION_DEBUG
			printf("parsed function token %#zx\n", (size_t) t);
			t ? printf("of type %d ident %s\n", t->type, 
			           ((t_func_data*)t->data)->ident) : printf("\n"); 
		#endif
		if(t->type < TokenDebugInvalid || t->type > TokenInvalid)
			break;
		t_htentry* entry = calloc(1, sizeof(t_htentry));
		entry->key = ((t_func_data*)t->data )->ident;
		entry->value = t;
		ht = hashtable_put(ht, entry);
	}
	returnToken->data = ht;
	return returnToken;
}

