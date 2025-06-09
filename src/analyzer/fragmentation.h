#ifndef FRAGMENTATION_H
#define FRAGMENTATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include "message.h"
#include "analyzer.h"

void* fragmentation_thread(const void* arg);

#endif
