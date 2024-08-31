#!/bin/bash
clear && gcc -std=c99 -pedantic -g -O0 -o compiler main.c tokenisation.h tokenisation.c hashtable.h hashtable.c codegen.h codegen.c && 
./compiler $1
