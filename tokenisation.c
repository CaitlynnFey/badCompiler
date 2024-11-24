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

const char *token_str_lookup[] = {"DEBUG INVALID", "assign", "scopen", "NULL", "intlit", "plus", "minus", "mul", "ret", "ident", "div", "declident", "funccall", "invaltoken"};

void token_destructor(t_token* t) {
	if (t == NULL) 
		return;
	token_destructor(t->children[0]);
	token_destructor(t->children[1]);
	free(t->data);
	free(t);
}

void debug_log_token(char* str, t_token* token) {
	if(!token) {
		printf("%s logging null token\n", str);
		return;
	}
	printf("\033[33;1m%s\033[0m\t\033[32;2m%p\033[0m\t", str, token);
	switch (token->type) {
		case TokenAssign:
			printf("\033[0;31massigning to\033[0m \033[34;1m%s\033[0m\n", (char*)token->data);
			break;
		case TokenIntLit:
			printf("\033[0;31mpushing\033[0m \033[34;1m%s\033[0m\n", (char*)token->data);
			break;
		case TokenPlus:
			printf("\033[0;31madding\033[0m \033[31;2m%p\033[0m and \033[31;2m%p\033[0m\n", token->children[0], token->children[1]);
			break;
		case TokenMinus:
			printf("\033[0;31msubtracting\033[0m \033[31;2m%p\033[0m and \033[31;2m%p\033[0m\n", token->children[0], token->children[1]);
			break;
		case TokenMul:
			printf("\033[0;31mmultiplying\033[0m \033[31;2m%p\033[0m and \033[31;2m%p\033[0m\n", token->children[0], token->children[1]);
			break;
		case TokenReturn:
			printf("\033[0;31mreturning\033[0m \033[31;2m%p\033[0m\n", token->children[0]);
			break;
		case TokenIdent:
			printf("\033[0;31mpushing\033[0m \033[34;1m%s\033[0m\n", (char*)token->data);
			break;
		case TokenDiv:
			printf("\033[0;31dmividing\033[0m \033[31;2m%p\033[0m and \033[31;2m%p\033[0m\n", token->children[0], token->children[1]);
			break;
		case TokenDeclIdent:
			printf("\033[0;31mdeclaring identifier\033[0m \"\033[34;1m%s\033[0m\"\n", (char*)token->data);
			break;
		case TokenFuncCall:
			printf("\033[0;31mcalling\033[0m \033[34;1m%s\033[0m\n", ((t_func_call*)token->data)->ident);
			break;
		
		default:
			printf("TokenType = %s, Children: %p (%s), %p (%s), Data: %.8s\n", 
					token_str_lookup[token->type], token->children[0],
					token->children[0] ? token_str_lookup[token->children[0]->type] : 0, 
					token->children[1], token->children[1] ? token_str_lookup[token->children[1]->type] :
					                            0, token->data ? (char*) token->data : "nil");
  	}
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

int ident_destructor(void* ident) {
	t_ident* idt = ident;
	free(idt->name);
	free(idt);
	return 0;
}

t_context* create_context(t_context* supctxt) {
	t_context* ctxt = calloc(1, sizeof(t_context));
	ctxt->identht = hashtable_create(&ident_destructor);
	ctxt->caster = supctxt;
	if(supctxt)
		supctxt->shadow = ctxt;
	return ctxt;
}

int context_destructor(void* context) {
	if(!context)
		return 0;
	t_context* ctxt = context;
	hashtable_destroy(ctxt->identht);
	t_context* shadow = ctxt->shadow;
	free(ctxt);
	return context_destructor(shadow);
}

void expect_consume_char(char** remaining, char c) {
	#ifdef TOKENISATION_DEBUG
		printf("consuming (expect) '%c' from \"%.3s...\"\n", c, *remaining);
	#endif
	if(**remaining != c) {
		fprintf(stderr, "Failed expect_consume_char with string \"%s\", expecting char '%c'\n", *remaining, c);
		exit(9);
	}
	*remaining += 1;
}

uint_least8_t try_consume_char(char** remaining, char c) {
	#ifdef TOKENISATION_DEBUG
		printf("consuming (try) '%c' from \"%.3s...\"\n", c, *remaining);
	#endif
	uint_least8_t ret = **remaining == c;
	if(ret)
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

uint_least8_t try_consume_chars(char** remaining, const char* str) {
	char* backup = *remaining;
	uint_least8_t ret = 1;
	for(size_t i = 0; i < strlen(str) && ret; i++) {
		WHITESPACE();
		ret &= try_consume_char(remaining, str[i]);
	}
	if(!ret)
		*remaining = backup;
	return ret;
}

uint_least8_t try_consume_keyword(char** remaining, const char* keyword) {
	#ifdef TOKENISATION_DEBUG
		printf("trying to consume keyword \"%s\" from \"%.16s\"\n", keyword, *remaining);
	#endif
	size_t keywordoffset = findKeywordPointerOffset(*remaining);
	// printf("%zu\n", keywordoffset);
	if(!keywordoffset)
		return 0;
	if(strncmp(*remaining, keyword, keywordoffset))
		return 0;
	*remaining += keywordoffset;
	WHITESPACE();
	return 1;
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

char* parse_ident(char** remaining) {
	#ifdef TOKENISATION_DEBUG
		printf("\033[0;34mparsing ident from \"%.8s\"\033[0m\n", *remaining);
	#endif
	size_t keywordoffset = findKeywordPointerOffset(*remaining);
	char* ret = calloc(keywordoffset + 1, sizeof(char));
	memcpy(ret, *remaining, keywordoffset);
	*remaining += keywordoffset;
	WHITESPACE();
	return ret;
}

t_statement_pointer* try_parse_arguments(char** remaining) {
	WHITESPACE();
	t_statement_pointer* args = NULL;
	t_statement_pointer* prev = NULL;
	while(1) {
		t_statement_pointer* ptr = calloc(1, sizeof(t_statement_pointer));

		ptr->statement = tryParseExpression(NULL, remaining, 0);
		if(!args)
			args = ptr;
		if(prev)
			prev->next = ptr;
		prev = ptr;
		WHITESPACE();
		if(!try_consume_char(remaining, ','))
			break;
	}
	WHITESPACE();
	expect_consume_char(remaining, ')');
	return args;
}

t_token* tryParseTerm(char** remaining) {
	#ifdef TOKENISATION_DEBUG 
		printf("tryParseTerm, %s", *remaining);
		getchar();
	#endif
	
	WHITESPACE();
	size_t intLitPointerOffset = findIntLitPointerOffset(*remaining);
	size_t identPointerOffset = findKeywordPointerOffset(*remaining);

	t_token* returnToken;
	returnToken = calloc(1, sizeof(t_token));
		
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
    char* ident = parse_ident(remaining);
    
    if(try_consume_char(remaining, '(')) {
    	WHITESPACE();
    	returnToken->type = TokenFuncCall;
    	t_func_call* data = calloc(1, sizeof(t_func_call));
    	data->ident = ident;
    	data->exprs = try_parse_arguments(remaining);
    	returnToken->data = data;
			#ifdef TOKENISATION_DEBUG
	    	printf("creating funccall token ident \"%s\", pointer %p\n", ident, returnToken);		
	    #endif
    	return returnToken;
    }

    returnToken->type = TokenIdent;
    returnToken->data = ident;
    
    #ifdef TOKENISATION_DEBUG
    	printf("creating ident token value \"%s\", pointer %p\n", (char*)returnToken->data, returnToken);		
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
	token_destructor(returnToken);
	return NULL;
}

// based on https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
t_token* tryParseExpression(t_token* token, char** remaining, int min_prec) { // mul parent
	#ifdef TOKENISATION_DEBUG 
		printf("tryParseExpression, %u, \"%s\"\n", min_prec, *remaining);
		getchar();
	#endif	

	WHITESPACE();
	//get lhs
	t_token* lhs = tryParseTerm(remaining); //should be either a bracketed expression or an int lit
	
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
			//turns out this was entirely justified
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
		printf("tryParseStatement, \"%s\"\n", *remaining);
		getchar();
	#endif	
	WHITESPACE();
	
	t_token* ret = calloc(1, sizeof(t_token));
	
	if(try_consume_keyword(remaining, "let")) {
		#ifdef TOKENISATION_DEBUG
			printf("let parsed!\n");
		#endif
		ret->type = TokenDeclIdent;
		ret->data = parse_ident(remaining);
		
		if(try_consume_char(remaining, '=')) {
			WHITESPACE();
			t_token* assign = calloc(1, sizeof(t_token));
			assign->type = TokenAssign;
			assign->data = ret->data;
			assign->children[0] = tryParseExpression(assign, remaining, 0);
			if(!assign->children[0]) {
				fprintf(stderr, "expected expression after '='!\n");
				exit(EXT_FAILURE_PARSING);
			}
			ret->children[0] = assign;
			#ifdef TOKENISATION_DEBUG
				printf("created declident child assign %p\n", assign);
			#endif
		}
		
		expect_consume_char(remaining, ';');
		#ifdef TOKENISATION_DEBUG
			printf("created declident token %p with ident '%s'\n", ret,  (char*) ret->data);
		#endif
		return ret;
	}
	
	if (try_consume_keyword(remaining, "return")) {
		#ifdef TOKENISATION_DEBUG
			printf("return parsed!\n");
		#endif
		ret->type = TokenReturn;
		ret->children[0] = tryParseExpression(ret, remaining, 0);
		if(!ret->children[0]) {
			fprintf(stderr, "failure parsing return! No Expr!\n");
			exit(EXT_FAILURE_PARSING);
		}
		WHITESPACE();
		expect_consume_char(remaining, ';');
		#ifdef TOKENISATION_DEBUG
			printf("created return token %p\n", ret);
		#endif
		return ret;
	}

	if(try_consume_char(remaining, '{')) {
		//TODO
	}

	if(try_consume_char(remaining, '}')) {
		return NULL;
	}

	char* ident = parse_ident(remaining);
	if(try_consume_char(remaining, '=')) {
		ret->type = TokenAssign;
		ret->data = ident;
		ret->children[0] = tryParseExpression(ret, remaining, 0);
		if(!ret->children[0]) {
				fprintf(stderr, "expected expression after '='!\n");
				exit(EXT_FAILURE_PARSING);
		}
		WHITESPACE();
		try_consume_char(remaining, ';');
		return ret;
	} else if (try_consume_char(remaining, '(')) {
		ret->type = TokenFuncCall;
		t_func_call* data = calloc(1, sizeof(t_func_call));
		
	}
	
	return ret;
	}

t_hashtable* tryParseIdentList(char** remaining, t_hashtable* ht) {
	size_t i = 0;
	do {
		WHITESPACE();
		if(**remaining == ')')
			break;
		size_t keywordoffset = findKeywordPointerOffset(*remaining);
		char* key = calloc(keywordoffset + 1, sizeof(char));
		memcpy(key, *remaining, keywordoffset );
		t_htentry* entry = calloc(1, sizeof (t_htentry));
		entry->key = key;
		entry->value = calloc(1, sizeof(size_t));
		*((size_t*) entry->value) = i;
		
		ht = hashtable_put(ht, entry);
		*remaining += keywordoffset;
		WHITESPACE();
		i++;
	} while (try_consume_char(remaining, ','));
	return ht;
}

t_func_data* tryParseFunction(char** remaining) {
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

	t_context* ctxt = create_context(NULL);
	tryParseIdentList(remaining, ctxt->identht);
	
	data->context = ctxt;
	data->args = data->context->identht->filled_cells;
	WHITESPACE();
	expect_consume_char(remaining, ')');
	WHITESPACE();	
	
	expect_consume_char(remaining, '{');

	t_statement_pointer* prev = NULL;	
	for(t_token* t = tryParseStatement(NULL, remaining); t; t = tryParseStatement(NULL, remaining)) {
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
	#ifdef TOKENISATION_DEBUG
		printf("created function %#zx ident %s with %zu args\n", (size_t)data, data->ident, data->args);
	#endif
	return data;
}

int func_destructor(void* func_data) {
	t_func_data* func = (t_func_data*) func_data;
	free(func->ident);
	context_destructor(func->context);
	for(t_statement_pointer* p = func->statements; p; p = p->next) {
		token_destructor(p->statement);
	}
	free(func);
	return 0;
}

t_prog_data* tokenise(t_prog_data* program, char** remaining) {
	t_func_ptr* ptr;
	t_func_ptr* prev = NULL;
	t_prog_data* ret = program ? program : calloc(1, sizeof(t_prog_data));
	ret->funcht = hashtable_create(&func_destructor);
	
	for(t_func_data* t = tryParseFunction(remaining); t; t = tryParseFunction(remaining)) {	
		t_func_ptr* cur = calloc(1, sizeof(t_func_ptr));
		cur->func = t;
		t_htentry* e = calloc(1, sizeof(t_htentry));
		e->key = cur->func->ident;
		e->value = cur->func;
		ret->funcht = hashtable_put(ret->funcht, e);
		if(prev) 
			prev->next = cur;
		else
			ptr = cur;
		prev = cur;
	}	
	ret->funcs = ptr;
	return ret;
}
