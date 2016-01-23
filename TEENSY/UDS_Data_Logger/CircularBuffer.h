/*
  * File Name: CircularBuffer.h
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

/* INCLUDES */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include "CANMessage.h"
#include "SDCard.h"

/* STRUCTS */
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
int circular_buffer_dump_to_file(circular_buffer_t *cb, SdFile *cbFile);

#endif // CIRCULARBUFFER_H

