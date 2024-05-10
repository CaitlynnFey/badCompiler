#include "codegen.h"
#include "hashtable.h"
#include "tokenisation.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_PUT_HT 99

void codegen_internal(t_token* cur_token, FILE* outfile, t_hashtable** ht, size_t* stacksize) {
  debug_log_token("cdgen", cur_token);
  switch(cur_token->type) {
    case TokenProg:
      fprintf(outfile, "global _start\n\n_start:\n");
      t_statement_pointer* p_ = cur_token->data;
      for (t_statement_pointer* p = p_->next; p->next || p->statement;) {
        printf("b\n");
        codegen_internal(p->statement, outfile, ht, stacksize);
        if(p->next) {
          p = p->next;
        } else {
          break;
        }
      }
     break; 
    case TokenReturn:
      //sys_exit %rax 60, %rdi error code
      codegen_internal(cur_token->children[0], outfile, ht, stacksize); //push return code onto stack
      fprintf(outfile, "\tpop rdi\n\tmov rax, 60\n\tsyscall\n"); //exit()
      *stacksize -= 1;
      break;
    case TokenIntLit:
      //printf("%s TokenIntLit: %s\n", debugshit, cur_token->data);
      fprintf(outfile, "\tmov rax, %s\n\tpush rax\n", (char*) cur_token->data);
      *stacksize += 1;
      break;
    case TokenPlus:
      codegen_internal(cur_token->children[0], outfile, ht, stacksize);
      codegen_internal(cur_token->children[1], outfile, ht, stacksize);
      fprintf(outfile, "\tpop rax\n\tpop rbx\n\tadd rax, rbx\n\tpush rax\n");
      *stacksize -= 1;
      break;
    case TokenMul:
      codegen_internal(cur_token->children[0], outfile, ht, stacksize);
      codegen_internal(cur_token->children[1], outfile, ht, stacksize);
      fprintf(outfile, "\tpop rax\n\tpop rbx\n\tmul rbx\n\tpush rax\n");
      *stacksize -= 1;
      break;
    case TokenDeclIdent:
      printf("tokendeclident\n");
      ;t_htentry* entry = malloc(sizeof(t_htentry));
      entry->key = cur_token->data;
      entry->value = malloc(sizeof(size_t));
      *((size_t*) entry->value) = *stacksize;
      *ht = hashtable_put(*ht, entry);
      fprintf(outfile, "\tinc rsp\n");
      *stacksize += 1;
      if(!ht)
        exit(EXT_PUT_HT);
      break;
    case TokenAssign:
      printf("tokenassign\n");
      ;size_t* stack_loc = hashtable_get(*ht, cur_token->data);
      if(!stack_loc) {
        fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: %s\033[0m\n", (char* )cur_token->data);
        exit(-1);
      }
      codegen_internal(cur_token->children[0], outfile, ht, stacksize);
      fprintf(outfile, "\tpop rax\n\tmov [rsp + %lu], rax\n", (*stacksize - *stack_loc - 1) * 8);
      *stacksize -= 1;
      break;
    case TokenIdent:
      printf("tokenident");
      ;size_t* stack_loc_ = hashtable_get(*ht, cur_token->data);
      if(!stack_loc_) {
        fprintf(stderr, "\033[0;31mExpected valid, declared ident. Got ident: %s\033[0m\n", (char* )cur_token->data);
        exit(-1);
      }    
      fprintf(outfile, "\tpush QWORD [rsp + %lu]\n", (*stacksize - *stack_loc - 1) * 8);
      *stacksize += 1; 
      break;
    default:
      *(volatile int*)0;
      fprintf(stderr, "Not implemented yet! %d\n\n", cur_token->type);
      exit(69);
  }
}

void codegen(t_token* cur_token, FILE* outfile) {
  t_hashtable* ht = hashtable_create();
  if(!ht)
    exit(99);
  printf("a\n");
  t_hashtable** ht_ref = malloc(sizeof(t_hashtable*));
  if(!ht_ref) {
    fprintf(stderr, "whar\n");
  }
  *ht_ref = ht;
  printf("a.25\n");
  size_t* stacksize = malloc(sizeof(size_t));
  memset(stacksize, 0, sizeof(size_t));
  printf("like\n");
  codegen_internal(cur_token, outfile, ht_ref, stacksize);
}
