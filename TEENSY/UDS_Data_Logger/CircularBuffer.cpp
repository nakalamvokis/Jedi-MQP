/*
  * File Name: CircularBuffer.cpp
  * Date: 1/21/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  *
  * 
*/

#include "CircularBuffer.h"

/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param capacity Size of circular buffer
 *  @param itemSize Size of a single item in the circular buffer
 */
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize)
{
  cb->bufferStart = (can_message_t *) calloc(capacity, itemSize);
  cb->capacity = capacity;
  cb->itemSize = itemSize;
  cb->head = cb->bufferStart;
  cb->tail = cb->bufferStart;
  cb->bufferEnd = cb->bufferStart;
  cb->hasWrapped = false;
  cb->bufferEnd += capacity;
}


/** Reinitializes circular buffer
 *  @param *cb Circular buffer struct to be reinitialized
 */
void circular_buffer_reinit(circular_buffer_t *cb)
{
  cb->head = cb->bufferStart;
  cb->tail = cb->bufferStart;
  cb->hasWrapped = false;
}


/** Frees circular buffer
 *  @param *cb Circular buffer struct to be freed
 */
void circular_buffer_free(circular_buffer_t *cb)
{
  free(cb->bufferStart);
}


/** Pushs a item onto the circular buffer
 *  @param *cb Circular buffer struct to be pushed to
 *  @param *item Data to be pushed to the circular buffer
 */
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(cb->head, item, cb->itemSize);
  cb->head++;
  if (cb->head == cb->bufferEnd)
  {
    cb->head = cb->bufferStart;
    cb->hasWrapped = true;
  }
}


/** Pops an item from the circular buffer
 *  @param *cb Circular buffer struct to be popped from
 *  @param *item Data to be populated with pop from circular buffer
 */
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(item, cb->tail, cb->itemSize);
  cb->tail++;
  if (cb->tail == cb->bufferEnd)
  {
    cb->tail = cb->bufferStart;
  }
}

