/*
  * File Name: LinearBuffer.h
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#ifndef LINEARBUFFER_H
#define LINEARBUFFER_H

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
  can_message_t *next;          // pointer to next free location
  size_t capacity;              // maximum number of items in buffer
  size_t itemSize;              // size of each item in buffer
  bool isFull;                  // whether or not the linear buffer is full
} linear_buffer_t;

/* FUNCTION PROTOTYPES */
void LinearBufferInit(linear_buffer_t *lb, size_t capacity, size_t itemSize);
void LinearBufferReinit(linear_buffer_t *lb);
void LinearBufferFree(linear_buffer_t *lb);
void LinearBufferPush(linear_buffer_t *lb, can_message_t *item);
uint16_t LinearBufferDumpToFile(linear_buffer_t *lb, SdFile *lbFile);

#endif // LINEARBUFFER_H

