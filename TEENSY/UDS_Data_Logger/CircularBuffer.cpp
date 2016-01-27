/*
  * File Name: CircularBuffer.cpp
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  *
*/

#include "CircularBuffer.h"

/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param capacity Size of circular buffer
 *  @param itemSize Size of a single item in the circular buffer
 */
void CircularBufferInit(circular_buffer_t *cb, size_t capacity, size_t itemSize)
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
void CircularBufferReinit(circular_buffer_t *cb)
{
  cb->head = cb->bufferStart;
  cb->tail = cb->bufferStart;
  cb->hasWrapped = false;
}

/** Frees circular buffer
 *  @param *cb Circular buffer struct to be freed
 */
void CircularBufferFree(circular_buffer_t *cb)
{
  free(cb->bufferStart);
}

/** Pushs a item onto the circular buffer
 *  @param *cb Circular buffer struct to be pushed to
 *  @param *item Data to be pushed to the circular buffer
 */
void CircularBufferPush(circular_buffer_t *cb, can_message_t *item)
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
void CicularBufferPop(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(item, cb->tail, cb->itemSize);
  cb->tail++;
  if (cb->tail == cb->bufferEnd)
  {
    cb->tail = cb->bufferStart;
  }
}

/** Dumps all circular buffer data to a file on the SD Card
 *  @param *cb Circular buffer struct to be dumped to SD
 *  @param *cbFile File to be written to
 *  @return messageCount Number of messages read from circular buffer
 */
uint16_t CircularBufferDumpToFile(circular_buffer_t *cb, SdFile *cbFile)
{ 
  can_message_t *readStart = NULL;
  can_message_t *readEnd = NULL;
  can_message_t *currentMessage = NULL;
  uint16_t messageCount = 0;

  if (cb->hasWrapped)
  {
    readStart = cb->head;
  }
  else
  {
    readStart = cb->bufferStart;
  }
  
  readEnd = cb->head;
  currentMessage = readStart;
  
  // iterate through circular buffer until we reach the end of the data
  while ((messageCount == 0) || (currentMessage != readEnd))
  {
    /* DIAG START */
    cbFile->print("MSG: ");
    cbFile->print(" ");
    cbFile->print(messageCount, DEC);
    cbFile->print(" ");
    /* DIAG END */
    FilePrintMessage(currentMessage, cbFile);
    currentMessage++;
    messageCount++;
    if(currentMessage == cb->bufferEnd)
    {
      currentMessage = cb->bufferStart;
    }
  }
  CircularBufferReinit(cb);
  return messageCount;
}

