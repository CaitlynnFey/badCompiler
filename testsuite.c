#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  FILE* file = fopen("tests.txt", "r");
  char line[256];
  char nextline[8];
  char command[512];
  if (file) {
    while (fgets(line, sizeof(line), file)) {
      sprintf(command, "./compiler ./tests/%s", line);
      system(command);
      memset(command, 0, 512);
      
      int pid = fork();
      if(pid == -1) {
        fprintf(stderr, "failed to fork!\n");
        return -1;
      }
      
      if(pid == 0) {
        execlp("./out", "", NULL);
      } else {
        int wstatus;
        wait(&wstatus);
        if(WIFEXITED(wstatus)) {
          int result = WEXITSTATUS(wstatus);
          fgets(nextline, sizeof(nextline), file);
          if(result != atoi(nextline)) {
            printf("\033[0;31mTest %s FAILED!, got %hhu, was expecting %u\033[0m\n", line, result, atoi(nextline));
          } else {
            printf("\033[32;3mTest %s passed.\033[0m\n", line);
          }
        }
      }
    }
    fclose(file);
  } else {
    fprintf(stderr, "failed to open file");
    return -1;
  }
  return 0;
}
