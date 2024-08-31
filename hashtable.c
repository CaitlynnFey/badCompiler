#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"
#include "tokenisation.h"

#define EXT_FAIL_MALLOC 8

// #define HT_DEBUG

uint64_t hash_fnv1a(char* data) {
	uint64_t hash = 0xcbf29ce484222325;
	for (int i = 0; i < strlen(data); i++) {
  	hash ^= (uint8_t) data[i];
		hash *= 0x100000001b3;
  }
	return hash;
}

t_hashtable* hashtable_create_internal(size_t size) {
	#ifdef HT_DEBUG
		printf("hashtable_create_internal(%zu)\n", size);
	#endif
	
	t_hashtable* ht = calloc(1, sizeof(t_hashtable));
	if(!ht) 
		return NULL;
	ht->size = size;
	ht->filled_cells = 0;
	ht->ht_begin = calloc(size, sizeof(t_htentry*));
	memset(ht->ht_begin, 0, ht->size);
	#ifdef HT_DEBUG
		printf("htpointer creation: %p\n", ht);
	#endif
	return ht;
}

t_hashtable* hashtable_create() {
	return hashtable_create_internal(16);
}
int free_element(t_hashtable* ht, t_htentry* entry) {
	if(ht->destructor_ptr(entry->value)) {
		fprintf(stderr, "failure to call destructor on entry with key %s\n", entry->key);
		return -1;
	}
	free(entry->key);
	free(entry);
	ht->filled_cells -= 1;
	return 0;	
}

int hashtable_remove(t_hashtable* ht, char* key) {
	if(!ht->destructor_ptr) {
		fprintf(stderr, "Attempted to call a nil destructor\n");
		return -1;
	}

	uint64_t hash = hash_fnv1a(key);
	for(size_t i = 0; i < ht->size; i++) {
		if(ht->ht_begin[(hash + i) % ht->size] && !strcmp(ht->ht_begin[(hash + i) % ht->size]->key, key)) {
			int ret = free_element(ht, ht->ht_begin[(hash + i) % ht->size]);
			ht->ht_begin[(hash + i) % ht->size] = NULL; //unsure how else to do this
			return ret;
		}
	}
	fprintf(stderr, "key '%s' not found\n", key);
	return 0;
}

void hashtable_destroy(t_hashtable* ht) {
	for(size_t i = 0; ht->filled_cells != 0; i++) {
		if(!ht->ht_begin[i])
			continue;
		free_element(ht, ht->ht_begin[i]);
		ht->ht_begin[i] = NULL;
	}
	free(ht);
}

int hashtable_copy(t_hashtable* src, t_hashtable* dest) {
	#ifdef HT_DEBUG
		printf("hashtable_copy(%p, %p)\n", src, dest);
	#endif

	if(src->size > dest->size)
		return -1;
	for (int i = 0; i > src->size; i++) {
		hashtable_put(dest, src->ht_begin[i]);
	}
	return 0;
} 

t_hashtable* hashtable_put_internal(t_hashtable* ht, t_htentry* entry, uint64_t hash, size_t tries) {
	#ifdef HT_DEBUG
		printf("hashtable_put_internal(%p, %p, 0x%xlu, %zu)\n", ht, entry, hash, tries);
	#endif
	if(ht->ht_begin[(tries + hash) % ht->size]) 
		return hashtable_put_internal(ht, entry, hash, tries + 1);
	
	ht->ht_begin[(tries + hash) % ht->size] = entry;
	
	#ifdef HT_DEBUG
		printf("put entry '%s' with value %p in %p after %zu tries with hash 0x%xlu\n", entry->key, entry->value, ht, tries, hash);
	#endif
	
	if(++ht->filled_cells > ht->size * 0.33f) {
		t_hashtable* new = hashtable_create_internal(ht->size*2);
		if(hashtable_copy(ht, new)) {
			fprintf(stderr, "failed to copy?!");
			exit(-1);
		}
		free(ht->ht_begin);
		free(ht);
		#ifdef HT_DEBUG
			printf("destroying ht %p to move to %p\n", ht, new);
		#endif
		return new;
	}
	return ht;
}

t_hashtable* hashtable_put(t_hashtable* ht, t_htentry* entry) {
	#ifdef HT_DEBUG
		printf("hashtable_put(%p, %p)\n", entry, entry);
		printf("entry: key '%s' value %d\n", entry->key, *(int*) entry->value);
	#endif
	if(!entry)
		return ht;
	
	uint64_t keyhash = hash_fnv1a(entry->key);
	return hashtable_put_internal(ht, entry, keyhash, 0);
}

t_htentry* hashtable_get_internal(t_hashtable* ht, char* key, uint64_t keyhash, size_t tries) {
	#ifdef HT_DEBUG
		printf("hashtable_get_internal(%p, '%s', %lu, %zu)\n", ht, key, keyhash, tries);
	#endif
	
	if(ht->ht_begin[(keyhash + tries) % ht->size]) {
		if(strcmp(ht->ht_begin[(keyhash + tries) % ht->size]->key, key))
			return hashtable_get_internal(ht, key, keyhash, tries + 1);	
		return ht->ht_begin[(keyhash + tries) % ht->size];
	}
	#ifdef HT_DEBUG
		printf("didn't find '%s' in %p after %zu with hash 0x%xlu\n", key, ht, tries, keyhash);
	#endif
	return NULL;
}

void* hashtable_get(t_hashtable* ht, char* key) {
	uint64_t keyhash = hash_fnv1a(key);

	t_htentry* entry = hashtable_get_internal(ht, key, keyhash, 0);
	if(!entry)
		return NULL;
	return entry->value;
}

