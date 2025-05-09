#ifndef BACKEND_H
#define BACKEND_H

#include "message.h"

void process_message(const Message* msg);
void finalize_client_report(int client_number);

#endif
