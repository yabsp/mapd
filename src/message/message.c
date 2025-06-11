#include "message.h"
#include "analyzer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/cJSON.h"

MessageQueue message_queue = {
    .head = 0, .tail = 0, .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER
};

void enqueue_message(const Message* msg) {
    pthread_mutex_lock(&message_queue.lock);

    if (message_queue.count < MAX_QUEUE_SIZE) {
        message_queue.messages[message_queue.tail] = *msg;
        message_queue.tail = (message_queue.tail + 1) % MAX_QUEUE_SIZE;
        message_queue.count++;
        pthread_cond_signal(&message_queue.not_empty);
    } else {
        fprintf(stderr, "[Queue] Message queue full! Dropping message.\n");
    }

    pthread_mutex_unlock(&message_queue.lock);
}

Message dequeue_message() {
    Message msg;
    pthread_mutex_lock(&message_queue.lock);

    while (message_queue.count == 0) {
        pthread_cond_wait(&message_queue.not_empty, &message_queue.lock);
    }

    msg = message_queue.messages[message_queue.head];
    message_queue.head = (message_queue.head + 1) % MAX_QUEUE_SIZE;
    message_queue.count--;

    pthread_mutex_unlock(&message_queue.lock);
    return msg;
}

Message parse_json_to_message(const char* json_str, int client_id) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.client_id = client_id;

    cJSON* root = cJSON_Parse(json_str);
    if (!root) return msg;


    cJSON* type = cJSON_GetObjectItem(root, "type");
    cJSON* addr = cJSON_GetObjectItem(root, "addr");
    cJSON* size = cJSON_GetObjectItem(root, "size");
    cJSON* thread = cJSON_GetObjectItem(root, "thread");
    cJSON* timestamp = cJSON_GetObjectItem(root, "timestamp");
    cJSON* severity = cJSON_GetObjectItem(root, "severity");
    cJSON* desc = cJSON_GetObjectItem(root, "description");

    if (type && cJSON_IsString(type)) strncpy(msg.type, type->valuestring, sizeof(msg.type));
    if (addr && cJSON_IsString(addr)) strncpy(msg.addr, addr->valuestring, sizeof(msg.addr));
    if (size && cJSON_IsNumber(size)) msg.size = size->valuedouble;
    if (thread && cJSON_IsNumber(thread)) msg.thread = thread->valuedouble;
    if (timestamp && cJSON_IsNumber(timestamp)) msg.timestamp = timestamp->valuedouble;
    if (severity && cJSON_IsString(severity)) strncpy(msg.severity, severity->valuestring, sizeof(msg.severity));
    if (desc && cJSON_IsString(desc)) strncpy(msg.description, desc->valuestring, sizeof(msg.description));

    cJSON_Delete(root);
    return msg;
}

void create_connection_message(int client_id, const char* event) {
    if (analyzer_options && analyzer_options->info_logs_enabled == 0)
        return;

    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.client_id = client_id;
    strncpy(msg.addr, "-", sizeof(msg.addr));
    msg.size = 0;
    msg.thread = (unsigned long)pthread_self();
    msg.timestamp = time(NULL);
    strncpy(msg.severity, "info", sizeof(msg.severity));

    if (strcmp(event, "connection") == 0) {
        strncpy(msg.type, "connection", sizeof(msg.type));
        snprintf(msg.description, sizeof(msg.description), "New connection: Client %d.", client_id);
    } else if (strcmp(event, "disconnection") == 0) {
        strncpy(msg.type, "disconnection", sizeof(msg.type));
        snprintf(msg.description, sizeof(msg.description), "Connection closed: Client %d.", client_id);
    }

    enqueue_message(&msg);
}


void message_free(Message *msg)
{
    if (!msg) return;
    free(msg);
}

Message* message_copy(const Message* src)
{
    if (!src) return NULL;

    Message* copy = malloc(sizeof(Message));
    if (!copy) return NULL;

    memcpy(copy, src, sizeof(Message));
    return copy;
}

