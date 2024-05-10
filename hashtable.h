#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

typedef struct s_htentry {
  char* key;
  void* value;
} t_htentry;

typedef struct s_ht {
  t_htentry** ht_begin;
  size_t size;
  size_t filled_cells;
} t_hashtable;

t_hashtable* hashtable_create();
void hashtable_destroy(t_hashtable* ht);
t_hashtable* hashtable_put(t_hashtable* ht, t_htentry* entry);
void* hashtable_get(t_hashtable* ht, char* key);
int hashtable_remove(t_hashtable*, char* key);
#endif
