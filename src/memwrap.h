#ifndef MEMWRAP_H
#define MEMWRAP_H

#include <stddef.h>

typedef enum {
    EVENT_MALLOC,
    EVENT_FREE,
    EVENT_MEMORY_LEAK,
    EVENT_DANGLING_POINTER,
    EVENT_BUFFER_OVERFLOW,
    EVENT_DOUBLE_FREE
} EventType;

const char* event_type_to_string(EventType type);
void send_json_event(EventType type, void* addr, size_t size);

#endif
