/*
  * @file CircularBuffer.h
  * @author Nicholas Kalamvokis
  * @date 1/21/2016
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
#include "Errors.h"

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
void CircularBufferInit(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void CircularBufferReinit(circular_buffer_t *cb);
void CircularBufferFree(circular_buffer_t *cb);
void CircularBufferPush(circular_buffer_t *cb, can_message_t *item);
void CicularBufferPop(circular_buffer_t *cb, can_message_t *item);
uint16_t CircularBufferDumpToFile(circular_buffer_t *cb, SdFile *cbFile);

#endif // CIRCULARBUFFER_H

