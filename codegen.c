#include "codegen.h"
#include "hashtable.h"
#include "tokenisation.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_PUT_HT 99

#define CDGEN_DEBUG

void codegen_internal(t_token* cur_token, FILE* outfile, t_hashtable** vars, t_hashtable* funcs, size_t* stacksize, size_t flags) {
  #ifdef CDGEN_DEBUG
    debug_log_token("cdgen", cur_token);
  #endif

  if(!cur_token)
    return;
  
  switch(cur_token->type) {
    case TokenReturn:
      {
        codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags); //push return code onto stack
        if(flags & 1) {
          fprintf(outfile, "\t;return (main)\n\tpop rdi\n\tmov rax, 60\n\tsyscall\n");
        } else {
          
        fprintf(outfile, "\t;return\n\tpop rax\n\tsub rsp, %li\n\tret\n", 8 * (*stacksize - 1));
        }
      }
      break;
      
    case TokenIntLit:
      {
        fprintf(outfile, "\t;intlit\n\tpush %s\n", (char*) cur_token->data);
        *stacksize += 1;
        if(cur_token->children[0])
          codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags);
      }
      break;
      
    case TokenPlus:
      {
        codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags);
        codegen_internal(cur_token->children[1], outfile, vars, funcs, stacksize, flags);
        fprintf(outfile, "\t;tokenplus\n\tpop rax\n\tpop rbx\n\tadd rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;
      
    case TokenMul:
      {
        codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags);
        codegen_internal(cur_token->children[1], outfile, vars, funcs, stacksize, flags);
        fprintf(outfile, "\t;tokenmul\n\tpop rax\n\tpop rbx\n\tmul rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;
      
    case TokenDeclIdent:
      {
        t_htentry* entry = malloc(sizeof(t_htentry));
        entry->key = cur_token->data;
        entry->value = calloc(1, sizeof(size_t));
        *((size_t*) entry->value) = *stacksize;
        *vars = hashtable_put(*vars, entry);
        fprintf(outfile, "\t;tokendeclident\n\tsub rsp, 8\n");
        *stacksize += 1;
        if(!vars)
          exit(EXT_PUT_HT);
        if(cur_token->children[0])
          codegen_internal(cur_token->children[0], outfile , vars, funcs, stacksize, flags);
      }
      break;
      
    case TokenAssign:
      {
        size_t* stack_loc = hashtable_get(*vars, cur_token->data);
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: '%s'\033[0m\n", (char* )cur_token->data);
          exit(16);
        }
        codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags);
        fprintf(outfile, "\t;tokenassign\n\tpop rax\n\tmov [rsp + %lu], rax\n", (*stacksize - *stack_loc - 1) * 8);
        *stacksize -= 1;
      }
      break;
      
    case TokenIdent:
      {
        size_t* stack_loc = hashtable_get(*vars, cur_token->data);
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: '%s'\033[0m %p\n", (char* )cur_token->data, stack_loc);
          exit(16);
        }    
        fprintf(outfile, "\t;tokenident\n\tpush QWORD [rsp + %lu]\n", (*stacksize - *stack_loc) * 8);
        *stacksize += 1; 
      }
      break;
      
    case TokenMinus:
      {
        codegen_internal(cur_token->children[0], outfile, vars, funcs, stacksize, flags);
        codegen_internal(cur_token->children[1], outfile, vars, funcs, stacksize, flags);
        fprintf(outfile, "\t;tokenminus\n\tpop rbx\n\tpop rax\n\tsub rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;    

    case TokenFuncCall:
      {
        for(t_statement_pointer* cur = ((t_func_call*) cur_token->data)->exprs; cur; cur = cur->next) {
          codegen_internal(cur->statement, outfile, vars, funcs, stacksize, flags);
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
    #ifdef CDGEN_DEBUG
      printf("\n\n\033[31;1mGenerating function \"%s\" with first statment 0x%p\033[0m\n\n", cur->func->ident, cur->func->statements->statement);
    #endif
    fprintf(outfile, "%s:\n", 
                strcmp(cur->func->ident, "main") ? 
                cur->func->ident : "_start");

    size_t stacksize = cur->func->identht->filled_cells;
    for (t_statement_pointer* p = cur->func->statements; p; p = p->next) {
      codegen_internal(p->statement, outfile, &cur->func->identht, prog_data->funcht, &stacksize, !strcmp(cur->func->ident, "main"));
    }
  }
}
