#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenisation.h"
#include "codegen.h"
#define EXT_SUCCESS 0
#define EXT_ERR_WRONG_ARGS_NUM 1
#define EXT_ERR_FAILURE_READING_SRC 2

int main(const int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Incorrect usage. Correct usage: #### /path/to/file ...\n");
    return EXT_ERR_WRONG_ARGS_NUM;
  }
  
  char** filebuffers = malloc(argc - 1);
  memset(&filebuffers, 0, argc - 1);
  FILE* srcFiles[argc - 1]; 
  
  for (int i = 0; i < argc - 1; i++) {
    srcFiles[i] = fopen(argv[i + 1], "r");
    if (!srcFiles[i])
      return EXT_ERR_FAILURE_READING_SRC;
    
    fseek(srcFiles[i], 0, SEEK_END);
    int length = ftell(srcFiles[i]);
    fseek (srcFiles[i], 0, SEEK_SET);
    filebuffers[i] = malloc(length + 1);
    fread(filebuffers[i], 1, length, srcFiles[i]);
    filebuffers[i][length] = '\0';
    fclose(srcFiles[i]);
  }
  
  for (int i = 0; i < argc - 1; i++) {
    char* a = filebuffers[i];
    t_prog_data* prog = tokenise(&a);
    FILE* outfile = fopen("out.asm", "w");
    codegen(prog, outfile);
    fclose(outfile);
  }

  system("nasm -felf64 out.asm");
  system("ld -o out out.o");
  
  return EXT_SUCCESS;
}
