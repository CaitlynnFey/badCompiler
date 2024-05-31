#include "codegen.h"
#include "hashtable.h"
#include "tokenisation.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_PUT_HT 99

// #define CDGEN_DEBUG

void codegen_internal(t_token* cur_token, FILE* outfile, t_hashtable** ht, size_t* stacksize) {
  #ifdef CDGEN_DEBUG
    debug_log_token("cdgen", cur_token);
  #endif
  
  switch(cur_token->type) {
    case TokenProg:
      {
        fprintf(outfile, "global _start\n\n_start:\n");
        t_statement_pointer* p_ = cur_token->data;
        for (t_statement_pointer* p = p_->next; p->next || p->statement;) {
          codegen_internal(p->statement, outfile, ht, stacksize);
          if(p->next) {
            p = p->next;
          } else {
            break;
          }
        }
     }
     break;
      
    case TokenReturn:
      {
        //sys_exit %rax 60, %rdi error code
        codegen_internal(cur_token->children[0], outfile, ht, stacksize); //push return code onto stack
        fprintf(outfile, "\t;return\n\tpop rdi\n\tmov rax, 60\n\tsyscall\n"); //exit()
        *stacksize -= 1;
      }
      break;
      
    case TokenIntLit:
      {
        fprintf(outfile, "\t;intlit\n\tpush %s\n", (char*) cur_token->data);
        *stacksize += 1;
      }
      break;
      
    case TokenPlus:
      {
        codegen_internal(cur_token->children[0], outfile, ht, stacksize);
        codegen_internal(cur_token->children[1], outfile, ht, stacksize);
        fprintf(outfile, "\t;tokenplus\n\tpop rax\n\tpop rbx\n\tadd rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;
      
    case TokenMul:
      {
        codegen_internal(cur_token->children[0], outfile, ht, stacksize);
        codegen_internal(cur_token->children[1], outfile, ht, stacksize);
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
        *ht = hashtable_put(*ht, entry);
        fprintf(outfile, "\t;tokendeclident\n\tsub rsp, 8\n");
        *stacksize += 1;
        if(!ht)
          exit(EXT_PUT_HT);
        if(cur_token->children[0])
          codegen_internal(cur_token->children[0], outfile , ht, stacksize);
      }
      break;
      
    case TokenAssign:
      {
        size_t* stack_loc = hashtable_get(*ht, cur_token->data);
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: '%s'\033[0m\n", (char* )cur_token->data);
          exit(-1);
        }
        codegen_internal(cur_token->children[0], outfile, ht, stacksize);
        fprintf(outfile, "\t;tokenassign\n\tpop rax\n\tmov [rsp + %lu], rax\n", (*stacksize - *stack_loc - 1) * 8);
        *stacksize -= 1;
      }
      break;
      
    case TokenIdent:
      {
        size_t* stack_loc = hashtable_get(*ht, cur_token->data);
        if(!stack_loc) {
          fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: %s\033[0m %p\n", (char* )cur_token->data, stack_loc);
          exit(-1);
        }    
        fprintf(outfile, "\t;tokenident\n\tpush QWORD [rsp + %lu]\n", (*stacksize - *stack_loc) * 8);
        *stacksize += 1; 
      }
      break;
      
    case TokenMinus:
      {
        codegen_internal(cur_token->children[0], outfile, ht, stacksize);
        codegen_internal(cur_token->children[1], outfile, ht, stacksize);
        fprintf(outfile, "\t;tokenminus\n\tpop rbx\n\tpop rax\n\tsub rax, rbx\n\tpush rax\n");
        *stacksize -= 1;
      }
      break;    
    default:
      fprintf(stderr, "Not implemented yet! %d %s\n", cur_token->type, token_str_lookup[cur_token->type]);
      *(volatile int*)0; //breakpoint
      exit(8);
  }
}

void codegen(t_token* cur_token, FILE* outfile) {
  t_hashtable* ht = hashtable_create();
  if(!ht)
    exit(99);
  t_hashtable** ht_ref = calloc(1, sizeof(t_hashtable*));
  if(!ht_ref) {
    fprintf(stderr, "failed to allocate hashtable reference\n");
    exit(99);
  }
  *ht_ref = ht;
  size_t* stacksize = malloc(sizeof(size_t));
  memset(stacksize, 0, sizeof(size_t));
  codegen_internal(cur_token, outfile, ht_ref, stacksize);
}
