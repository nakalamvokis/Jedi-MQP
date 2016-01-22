/*
  * File Name: CANMessage.cpp
  * Date: 1/21/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  *
  * 
*/

#include "CANMessage.h"


/** Initializes CAN configuration parameters
 *  @param canConfig CAN configuration struct to be set
 */
void can_config_init(FLEXCAN_config_t *canConfig)
{
  canConfig->presdiv   = 1;  // Prescale division factor.
  canConfig->propseg   = 2;  // Prop Seg length.
  canConfig->rjw       = 1;  // Sychronization Jump Width
  canConfig->pseg_1    = 7;  // Phase 1 length
  canConfig->pseg_2    = 3;  // Phase 2 length
}


/** Reads a CAN message from the FIFO queue
 *  @param *frame Pointer to frame struct to be populated with message data
 *  @return Whether or not a message was read from the FIFO queue
 */
bool can_fifo_read(FLEXCAN_frame_t *frame)
{
   if(FLEXCAN_fifo_avalible())
   {
      FLEXCAN_fifo_read(frame);
      return true;
   }
   return false;
}


/** Prints a message to the serial port
 *  @param message Message struct containing message data
 */
void serial_print_can_message(can_message_t *message)
{
  uint8_t currentData = 0;
  Serial.print(message->timestamp, DEC);
  Serial.print(" ");
  Serial.print(message->id, HEX);
  Serial.print(" ");
  for (currentData = 0; currentData < message->len; currentData++)
  {
    Serial.print(message->data[currentData], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


/** Prints a frame to the serial port
 *  @param frame Frame struct containing message data
 */
void serial_print_frame(FLEXCAN_frame_t *frame)
{
  int frameLength = frame->dlc;
  int dataIndex = 0;
  Serial.print(millis(), DEC);
  Serial.print(" ");
  Serial.print(frame->id, HEX);
  Serial.print(" ");
  for (dataIndex = 0; dataIndex < frameLength; dataIndex++)
  {
    Serial.print(frame->data[dataIndex]);
    Serial.print(" ");
  }
  Serial.println();
}


/** Generates a random CAN frame
 *  This function will only be used for testing purposes
 * @param frame Frame to be filled with random data
 * @param minID Minimim ID of generated frame
 * @param maxID Maximum ID of generated frame
 * @return If a frame was generated or not
 */
int generate_frame(FLEXCAN_frame_t *frame, uint16_t minID, uint16_t maxID)
{
  uint8_t status = random(0, 1 + 1);
  if (status)
  {
    int dataCounter = 0;
    frame->dlc = random(0, 8 + 1);
    for (dataCounter = 0; dataCounter < frame->dlc; dataCounter++)
    {
      frame->data[dataCounter] = random(0, 255 + 1);
    }
    frame->id = random(minID, maxID + 1);
  }
  return status;
}

/** Copies frame data to a CAN message
 *  This will be used to extract neccessary data from a frame in order to shorten the
 *  size of each packet in memory
 *  @param *message CAN message to be filled with data
 *  @param *frame Frame to be copied to CAN message
 */
void transpose_can_message(can_message_t *message, FLEXCAN_frame_t *frame)
{
  message->timestamp = millis();
  memcpy(&(message->len), &(frame->dlc), sizeof(frame->dlc));
  memcpy(&(message->id), &(frame->id), sizeof(frame->id));
  memcpy(&(message->data), &(frame->data), sizeof(frame->data));
}

