/*
  * File Name: LinearBuffer.cpp
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
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


/** Dumps all linear buffer data to an open file on the SD Card
 *  @param *lb Linear buffer struct to be dumped to SD
 *  @param *lbFile File to be written to
 *  @return messageCount Number of messages read from linear buffer
 */
int linear_buffer_dump_to_file(linear_buffer_t *lb, SdFile *lbFile)
{
  uint16_t messageCount = 0;
  can_message_t *currentMessage = lb->bufferStart;
  
  // iterate through linear buffer until we reach the end of the data
  while ((messageCount == 0) || (currentMessage != lb->bufferEnd))
  {
    /* DIAG START */
    lbFile->print("MSG: ");
    lbFile->print(" ");
    lbFile->print(messageCount, DEC);
    lbFile->print(" ");
    /* DIAG END */
    file_print_message(currentMessage, lbFile);
    currentMessage++;
    messageCount++;
  }
  return messageCount;
}

