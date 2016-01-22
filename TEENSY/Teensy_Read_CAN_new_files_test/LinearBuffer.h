#ifndef LINEARBUFFER_H
#define LINEARBUFFER_H

/* INCLUDES */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>

void linear_buffer_init(linear_buffer_t *lb, size_t capacity, size_t itemSize);
void linear_buffer_reinit(linear_buffer_t *lb);
void linear_buffer_free(linear_buffer_t *lb);
void linear_buffer_push(linear_buffer_t *lb, can_message_t *item);

#endif // CIRCULARBUFFER_H
