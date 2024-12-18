#include "codegen.h"
#include "hashtable.h"
#include "tokenisation.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_PUT_HT 99

// #define CDGEN_DEBUG

void codegen_internal(	t_token* cur_token, FILE* outfile, t_hashtable* funcs,
						size_t* stacksize, t_context* ctxt, t_func_data* cur_func, size_t flags) {
  #ifdef CDGEN_DEBUG
    debug_log_token("cdgen", cur_token);
  #endif

  if(!cur_token)
    return;
  
  switch(cur_token->type) {
	case TokenScope: 
		{	
      for (t_statement_pointer* p = ((t_scope_data*)cur_token->data)->stmts; p; p = p->next) {
        codegen_internal(p->statement, outfile, funcs, stacksize, ((t_scope_data*)cur_token->data)->ctxt, cur_func,flags); 
    }  
  }
		break;
	
    case TokenReturn:
      {
        codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags); //push return code onto stack
        if(flags & 1) { // if handling the main function
          // call sys_exit with the correct error code
		  fprintf(outfile, "\t;return (main)\n\tpop rdi\n\tmov rax, 60\n\tsyscall\n"); 
        } else { 
			// otherwise return to the caller.
			fprintf(outfile, "\t;return\n\tpop rax\n\tadd rsp, %li\n\tret\n", 
			8 * (ctxt->identht->filled_cells - cur_func->args));
        }
      }
      break;
      
    case TokenIntLit:
      {
        fprintf(outfile, "\t;intlit\n\tpush %s\n", (char*) cur_token->data);
        *stacksize += 1;
        if(cur_token->children[0])
          codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
      }
      break;
      
    case TokenPlus:
      {
        codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
        codegen_internal(cur_token->children[1], outfile, funcs, stacksize, ctxt, cur_func, flags);
        fprintf(outfile, "\t;tokenplus\n\tpop rax\n\tpop rbx\n\tadd rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;
      
    case TokenMul:
      {
        codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
        codegen_internal(cur_token->children[1], outfile, funcs, stacksize, ctxt, cur_func, flags);
        fprintf(outfile, "\t;tokenmul\n\tpop rax\n\tpop rbx\n\tmul rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;
      
    case TokenDeclIdent:
      {
		  //TODO this needs to use the 
        t_htentry* entry = malloc(sizeof(t_htentry));
        entry->key = cur_token->data;
        entry->value = calloc(1, sizeof(size_t));
        *((size_t*) entry->value) = *stacksize;
        cur_func->context->identht = hashtable_put(cur_func->context->identht, entry);
        fprintf(outfile, "\t;tokendeclident\n\tsub rsp, 8\n");
        *stacksize += 1;
        if(cur_token->children[0])
          codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
      }
      break;
      
    case TokenAssign:
      {
        size_t* stack_loc = hashtable_get(cur_func->context->identht, cur_token->data);
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: '%s'\033[0m\n", (char* )cur_token->data);
          exit(16);
        }
        codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
        fprintf(outfile, "\t;tokenassign\n\tpop rax\n\tmov [rsp + %lu], rax\n", (*stacksize - *stack_loc - 2) * 8);
        *stacksize -= 1;
      }
      break;
      
    case TokenIdent:
      {
        size_t* stack_loc = hashtable_get(cur_func->context->identht, cur_token->data);
        #ifdef CDGEN_DEBUG
          printf("var %s is at stack loc %lu, cur stack loc %lu\n", (char*)cur_token->data, *stack_loc, *stacksize);
        #endif
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: '%s'\033[0m %p\n", (char* )cur_token->data, stack_loc);
          exit(16);
        }
        fprintf(outfile, "\t;tokenident\n\tpush QWORD [rsp + %lu]\n", (*stacksize - *stack_loc - 1) * 8);
        *stacksize += 1; 
      }
      break;
      
    case TokenMinus:
      {
        codegen_internal(cur_token->children[0], outfile, funcs, stacksize, ctxt, cur_func, flags);
        codegen_internal(cur_token->children[1], outfile, funcs, stacksize, ctxt, cur_func, flags);
        fprintf(outfile, "\t;tokenminus\n\tpop rbx\n\tpop rax\n\tsub rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;    

    case TokenFuncCall:
      {
        for(t_statement_pointer* cur = ((t_func_call*) cur_token->data)->exprs; cur; cur = cur->next) {
          codegen_internal(cur->statement, outfile, funcs, stacksize, ctxt, cur_func, flags);
        }
        fprintf(outfile, "\tcall %s\n\tpush rax\n",  ((t_func_call*) cur_token->data)->ident);
        *stacksize += 1;
      }
      break;
      
    default:
      fprintf(stderr, "Not implemented yet! %d %s\n", cur_token->type, token_str_lookup[cur_token->type]);
      // *(volatile int*)0; //breakpoint
      exit(8);
  }
}

void codegen(t_prog_data* prog_data, FILE* outfile) {
  fprintf(outfile, "global _start\n\n");
  for(t_func_ptr* cur = prog_data->funcs; cur; cur = cur->next) {
    fprintf(outfile, "%s:\n", 
                strcmp(cur->func->ident, "main") ? 
                cur->func->ident : "_start");

    size_t stacksize = cur->func->context->identht->filled_cells + (strcmp(cur->func->ident, "main") != 0);

    #ifdef CDGEN_DEBUG
      printf("\n\n\033[31;1mGenerating function \"%s\" with stacksize %lu\033[0m\n\n", cur->func->ident, stacksize);
    #endif
    for (t_statement_pointer* p = cur->func->statements; p; p = p->next) {
      codegen_internal(p->statement, outfile, prog_data->funcht, &stacksize, cur->func->context, cur->func,!strcmp(cur->func->ident, "main"));
    }
  }
}
