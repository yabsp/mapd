#ifndef MESSAGE_H
#define MESSAGE_H

#include <stddef.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int client_id;
    char type[32];
    char addr[32];
    size_t size;
    unsigned long thread;
    time_t timestamp;
    char severity[16];
    char description[128];
} Message;

#define MAX_QUEUE_SIZE 1024

typedef struct {
    Message messages[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} MessageQueue;

extern MessageQueue message_queue;

void enqueue_message(const Message* msg);
Message dequeue_message();
Message parse_json_to_message(const char* json_str, int client_id);
void message_free(Message* msg);
Message* message_copy(const Message* src);
void create_connection_message(int client_id, const char* event);

#endif
