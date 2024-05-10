#!/bin/bash
clear && gcc -std=c99 -pedantic -g -O0 main.c tokenisation.h tokenisation.c hashtable.h hashtable.c codegen.h codegen.c && ./a.out examplecode.l && ./out
