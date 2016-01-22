/*
  * File Name: LinearBuffer.cpp
  * Date: 1/21/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  *
  * 
*/

#include "LinearBuffer.h"

/** Initializes linear buffer
 *  @param *lb Linear buffer struct to be initialized
 *  @param capacity Size of linear buffer
 *  @param itemSize Size of a single item in the linear buffer
 */
void linear_buffer_init(linear_buffer_t *lb, size_t capacity, size_t itemSize)
{
  lb->bufferStart = (can_message_t *) calloc(capacity, itemSize);
  lb->capacity = capacity;
  lb->itemSize = itemSize;
  lb->next = lb->bufferStart;
  lb->bufferEnd = lb->bufferStart;
  lb->bufferEnd += capacity;
  lb->isFull = false;
}


/** Reinitializes linear buffer
 *  @param *cb Linear buffer struct to be reinitialized
 */
void linear_buffer_reinit(linear_buffer_t *lb)
{
  lb->next = lb->bufferStart;
  lb->isFull = false;
}


/** Frees linear buffer
 *  @param *lb Linear buffer struct to be freed
 */
void linear_buffer_free(linear_buffer_t *lb)
{
  free(lb->bufferStart);
}


/** Pushs a value onto the linear buffer
 *  @param *lb Linear buffer struct to be pushed to
 *  @param *item Data to be pushed to the linear buffer
 */
void linear_buffer_push(linear_buffer_t *lb, can_message_t *item)
{
  memcpy(lb->next, item, lb->itemSize);
  lb->next++;
  if (lb->next == lb->bufferEnd)
  {
    lb->next = lb->bufferStart;
    lb->isFull = true;
  }
}

