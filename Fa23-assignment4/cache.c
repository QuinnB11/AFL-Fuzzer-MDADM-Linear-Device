#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cache.h"
#include "jbod.h"

//Uncomment the below code before implementing cache functions.
static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
    if (num_entries > 4096 || num_entries < 2) {
      return -1;
    }
    if (!cache_enabled()) {
      cache = (cache_entry_t*) calloc(num_entries,sizeof(cache_entry_t));
      cache_size += num_entries;
      int i;
      for(i=0;i<cache_size;++i) {
        cache[i].valid = false;
      }
      return 1;
    }
    return -1;
}

int cache_destroy(void) {
  if (cache_enabled()) {
    free(cache);
    cache = NULL;
    cache_size = 0;
    clock = 0;
    return 1;
  }
  return -1;
}

bool cache_empty(void) {
  if (cache_enabled()) {
    if (cache[0].valid) {
      return false;
    }
  }
  return true;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  if (buf == NULL || !cache_enabled() || cache_empty()) {
    return -1;
  }
  num_queries += 1;
  int i = 0;
  bool found = false;
  for(i=0;i<cache_size;i++) {
    if(cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      memcpy(buf, cache[i].block, 256);
      num_hits += 1;
      cache[i].clock_accesses = clock;
      clock += 1;
      found = true;
    }
  }
  if(found) {
    return 1;
  }
  return -1;
}



void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  int i;
  for(i=0;i<cache_size;i++) {
    if(cache[i].disk_num == disk_num && cache[i].block_num == block_num) {
      memcpy(cache[i].block, buf, 256);
      cache[i].clock_accesses = clock;
      clock += 1;
    }
  } 
}

int cache_min() {
  int i, max, maxIndex;
  max = cache[0].clock_accesses;
  maxIndex = 0;
  for(i=1;i<cache_size;i++) {
    if(cache[i].clock_accesses > max) {
      max = cache[i].clock_accesses;
      maxIndex = i;
    }
  }
  return maxIndex;
}

bool cache_full(void) {
  int i;
  for(i=0;i<cache_size;i++) {
    if(!cache[i].valid) {
      return false;
    }
  }
  return true;
}

bool alreadyIn_Cache(int disk_num,int block_num, const uint8_t *buf) {
  int i;
  for(i=0;i<cache_size;i++) {
    if(cache[i].disk_num == disk_num && cache[i].block_num == block_num &&  memcmp(cache[i].block, buf, 256) == 0) {     
      return true;
    }
  }
  return false;
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  if (!cache_enabled() || buf == NULL || block_num > 256 || disk_num > 16 || disk_num < 0 || block_num < 0 || alreadyIn_Cache(disk_num,block_num, buf)) {
    return -1;
  }

  if (cache_full()) {
    int minIndex = cache_min();
    cache[minIndex].disk_num = disk_num;
    cache[minIndex].block_num = block_num;
    memcpy(cache[minIndex].block, buf, 256);
    cache[minIndex].clock_accesses = clock;
    clock += 1;
    return 1;
  }
  int i;
  for(i=0;i<cache_size;i++) {
    if (!cache[i].valid) {
      cache[i].valid = true;
      cache[i].disk_num = disk_num;
      cache[i].block_num = block_num;
      cache[i].clock_accesses = clock;
      clock += 1;
      memcpy(cache[i].block, buf, 256);
      break;
    }
  }

  return 1;
}


bool cache_enabled(void) {
  if (cache_size == 0) {
    return false;
  }
  return true;
}

void cache_print_hit_rate(void) {
	fprintf(stderr, "num_hits: %d, num_queries: %d\n", num_hits, num_queries);
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}

int cache_resize(int new_num_entries) {
   cache = (cache_entry_t*) realloc(cache, new_num_entries);
   cache_size = new_num_entries;
   return 1;
}
