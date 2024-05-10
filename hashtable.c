#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

#define EXT_FAIL_MALLOC 8

uint64_t FNV1a(char* data) {
	uint64_t hash = 0xcbf29ce484222325;
	for (int i = 0; i < strlen(data); i++) {
  	hash ^= (uint8_t) data[i];
		hash *= 0x100000001b3;
  }
	return hash;
}

t_hashtable* hashtable_create_internal(size_t size) {
	printf("hashtable_create_internal\n");
	t_hashtable* ht = malloc(sizeof(t_hashtable));
	if(!ht) 
		return NULL;
	ht->size = size;
	printf("1\n");
	ht->filled_cells = 0;
	ht->ht_begin = malloc(size * sizeof(t_htentry*));
	memset(ht->ht_begin, 0, ht->size);
	printf("2\n");
	printf("htpointer creation: %p\n", ht);
	return ht;
}

t_hashtable* hashtable_create() {
	printf("hashtable_create\n");
	return hashtable_create_internal(16);
}

void hashtable_destroy(t_hashtable* ht) {
	//dont care rn
}

int hashtable_copy(t_hashtable* src, t_hashtable* dest) {
	printf("hashtable_copy\n");
	if(src->size > dest->size)
		return -1;
	for (int i = 0; i > src->size; i++) {
		hashtable_put(dest, src->ht_begin[i]);
	}
	return 0;
} 

t_hashtable* hashtable_put_internal(t_hashtable* ht, t_htentry* entry, uint64_t hash, size_t tries) {
	printf("hashtableputinternal: %p\n", ht);
	if(ht->ht_begin[(tries + hash % ht->size) % ht->size]) 
		return hashtable_put_internal(ht, entry, hash, tries + 1);
	ht->ht_begin[(tries + hash % ht->size) % ht->size] = entry;
	if(++ht->filled_cells > ht->size * 0.33f) {
		t_hashtable* new = hashtable_create_internal(ht->size*2);
		if(hashtable_copy(ht, new)) {
			fprintf(stderr, "failed to copy?!");
			exit(-1);
		}
		free(ht->ht_begin);
		free(ht);
		return new;
	}
	return ht;
}

t_hashtable* hashtable_put(t_hashtable* ht, t_htentry* entry) {
	printf("hashtable_put: %s, %lu\n", entry->key, *(size_t*)entry->value);
	if(!entry)
		return ht;
	
	uint64_t keyhash = FNV1a(entry->key);
	return hashtable_put_internal(ht, entry, keyhash, 0);
}

t_htentry* hashtable_get_internal(t_hashtable* ht, char* key, uint64_t keyhash, size_t tries) {
	printf("hashtablegetinternal\n");
	if(!(ht->ht_begin[((keyhash % ht->size) + tries) % ht->size]))
		return NULL;
	if(ht->ht_begin[((keyhash % ht->size) + tries) % ht->size]->key == key)
		return ht->ht_begin[((keyhash % ht->size) + tries) % ht->size];
	return hashtable_get_internal(ht, key, keyhash, tries + 1);
	}

void* hashtable_get(t_hashtable* ht, char* key) {
	uint64_t keyhash = FNV1a(key);

	t_htentry* entry= hashtable_get_internal(ht, key, keyhash, 0);
	if(!entry)
		return entry;
	return entry->value;
}

int hashtable_remove(t_hashtable* ht, char* key) {
	t_htentry* entry = hashtable_get_internal(ht, key, FNV1a(key), 0);
	if(!entry)
		return -1;
	free(entry->key);
	memset(entry, 0, 1);
	return 0;
}
