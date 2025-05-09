#include "backend.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_CLIENTS 32
#define MAX_ALLOCS_PER_CLIENT 8192

typedef struct {
  char addr[32];
  size_t size;
  bool freed;
  bool double_freed;
} Allocation;

typedef struct {
  Allocation allocations[MAX_ALLOCS_PER_CLIENT];
  int count;
} ClientAllocations;

static ClientAllocations client_data[MAX_CLIENTS];

static Allocation* find_allocation(ClientAllocations* ca, const char* addr) {
  for (int i = 0; i < ca->count; i++) {
    if (strcmp(ca->allocations[i].addr, addr) == 0) {
      return &ca->allocations[i];
    }
  }
  return NULL;
}

void process_message(const Message* msg) {
  if (msg->client_id < 0 || msg->client_id >= MAX_CLIENTS) return;

  ClientAllocations* ca = &client_data[msg->client_id];

  if (strcmp(msg->type, "malloc") == 0) {
    if (ca->count >= MAX_ALLOCS_PER_CLIENT) return;
    Allocation* a = &ca->allocations[ca->count++];
    strncpy(a->addr, msg->addr, sizeof(a->addr));
    a->size = msg->size;
    a->freed = false;
    a->double_freed = false;

  } else if (strcmp(msg->type, "free") == 0) {
    Allocation* a = find_allocation(ca, msg->addr);
    if (a) {
      if (a->freed) {
        fprintf(stderr, "[Client %d] Double free detected at %s\n", msg->client_id, msg->addr);
        a->double_freed = true;
      } else {
        a->freed = true;
      }
    } else {
      fprintf(stderr, "[Client %d] Freeing unknown address %s\n", msg->client_id, msg->addr);
    }

  } else if (strcmp(msg->type, "dangling_pointer") == 0) {
    fprintf(stderr, "[Client %d] Dangling access at %s\n", msg->client_id, msg->addr);
  } else if (strcmp(msg->type, "buffer_overflow") == 0) {
    fprintf(stderr, "[Client %d] Buffer overflow detected at %s\n", msg->client_id, msg->addr);
  } else if (strcmp(msg->type, "double_free") == 0) {
    fprintf(stderr, "[Client %d] Double free reported externally at %s\n", msg->client_id, msg->addr);
  }
}

void finalize_client_report(int client_id) {
  if (client_id < 0 || client_id >= MAX_CLIENTS) return;

  ClientAllocations* ca = &client_data[client_id];

  int leaks = 0, double_frees = 0, total = ca->count;
  size_t leaked_bytes = 0;

  for (int i = 0; i < ca->count; i++) {
    if (!ca->allocations[i].freed) {
      leaks++;
      leaked_bytes += ca->allocations[i].size;
    }
    if (ca->allocations[i].double_freed) {
      double_frees++;
    }
  }

  printf("\n===== Client %d Memory Report ======\n", client_id);
  printf("Total allocations: %d\n", total);
  printf("Freed: %d\n", total-leaks);
  printf("Leaks: %d (%.1f KB)\n", leaks, leaked_bytes / 1024.0);
  printf("Double frees: %d\n", double_frees);

  if (leaks == 0) printf("No memory leaks.\n");
  else printf("Memory leaks detected.\n");

  printf("=====================================\n");

  memset(ca, 0, sizeof(ClientAllocations)); // Reset for reuse
}