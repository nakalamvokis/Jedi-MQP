#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

/* INCLUDES */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>

/* STRUCTS */
typedef struct {
  uint32_t timestamp; // Timestamp - milliseconds since runtime
  uint8_t len;        // Data length code [0 - 8]
  uint32_t id;        // Arbitration ID
  uint8_t data[8];    // Data payload - maximum 8 bytes
} can_message_t;

typedef struct {
  can_message_t *bufferStart;   // start of data buffer
  can_message_t *bufferEnd;     // end of data buffer
  can_message_t *head;          // pointer to head
  can_message_t *tail;          // pointer to tail
  size_t capacity;              // maximum number of items in buffer
  size_t itemSize;              // size of each item in buffer
  bool hasWrapped;              // whether or not circular buffer has bufferEndped around
} circular_buffer_t;


/* FUNCTION PROTOTYPES */
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void circular_buffer_reinit(circular_buffer_t *cb);
void circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item);
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item);

#endif // CIRCULARBUFFER_H
