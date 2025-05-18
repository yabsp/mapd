#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include "message.h"
#include "fragmentation.h"

// Unclear what realistic values might be

// These provide a positive test
//#define FRAG_SMALL_THRESHOLD 3500
//#define FRAG_LARGE_THRESHOLD 200

// Values for real world application
#define FRAG_SMALL_THRESHOLD 3500
#define FRAG_LARGE_THRESHOLD 100

void* fragmentation_thread(void* arg) {
  (void)arg;

  while (1) {
    FILE* fp = fopen("/proc/buddyinfo", "r");
    if (!fp) {
      perror("[Fragmentation] Failed to open /proc/buddyinfo");
      sleep(10);
      continue;
    }

    char line[512];
    int small_blocks = 0;
    int large_blocks = 0;

    while (fgets(line, sizeof(line), fp)) {
      int order_counts[11] = {0};
      int parsed = sscanf(line,
                  "Node %*d, zone %*s %d %d %d %d %d %d %d %d %d %d %d",
                  &order_counts[0], &order_counts[1], &order_counts[2],
                  &order_counts[3], &order_counts[4], &order_counts[5],
                  &order_counts[6], &order_counts[7], &order_counts[8],
                  &order_counts[9], &order_counts[10]);

      if (parsed == 11) {
        for (int i = 0; i <= 4; i++) small_blocks += order_counts[i];
        for (int i = 5; i <= 10; i++) large_blocks += order_counts[i];
      }
    }
    fclose(fp);

    // Debug print
    fprintf(stderr, "[Fragmentation] small_blocks=%d, large_blocks=%d\n", small_blocks, large_blocks);

    if (large_blocks < FRAG_LARGE_THRESHOLD && small_blocks > FRAG_SMALL_THRESHOLD) {
      Message msg;
      memset(&msg, 0, sizeof(Message));
      msg.client_id = -1;
      strncpy(msg.type, "fragmentation", sizeof(msg.type));
      strncpy(msg.addr, "system", sizeof(msg.addr));
      msg.size = small_blocks;
      msg.thread = (unsigned long) pthread_self();
      msg.timestamp = time(NULL);
      strncpy(msg.severity, "warning", sizeof(msg.severity));
      strncpy(msg.description, "System memory fragmentation detected.", sizeof(msg.description));

      enqueue_message(&msg);
      fprintf(stderr, "[Fragmentation] Fragmentation event enqueued!\n");
    }

    sleep(10);

  }

  return NULL;
}