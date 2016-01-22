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
void linear_buffer_init(linear_buffer_t *lb, size_t capacity, size_t itemSize);
void linear_buffer_reinit(linear_buffer_t *lb);
void linear_buffer_free(linear_buffer_t *lb);
void linear_buffer_push(linear_buffer_t *lb, can_message_t *item);

#endif // LINEARBUFFER_H

