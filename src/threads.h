#ifndef THREADS_H
#define THREADS_H

#include <stdbool.h>

extern bool working;
extern bool aborted;

void abort_threads(void);

void dorip(void);

#endif
