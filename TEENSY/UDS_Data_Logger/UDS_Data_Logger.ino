/*
  * File Name: UDS_DATA_LOGGER.ino
  * Date: 1/21/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  *
  * 
*/

/* INCLUDES */
#include <SPI.h>
#include <can.h>
#include <SdFat.h>
#include <Time.h>
#include "CircularBuffer.h"
#include "LinearBuffer.h"
#include "CANMessage.h"
#include "SDCard.h"


/* FUNCTION PROTOTYPES */
void create_file_timestamp(char *timestamp, size_t strLen);
int circular_buffer_dump(circular_buffer_t *cb);
int linear_buffer_dump(linear_buffer_t *lb, SdFile *lbFile);
void changeState(int newReadType, int newNetworkStatus);

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY      2000      // Maximum capacity of the circular buffer
#define LINEAR_BUFFER_CAPACITY        1000      // Maximum capacity of the linear buffer
#define UDS_ID                        0x7E8     // Arbitration ID of all UDS messages sent to the vehicle
#define TEST_PACKET_TRANSFER_DELAY    1         // Packet simulation send delay time
#define CIRCULAR_BUFFER               0         // Store messages into circular buffer
#define LINEAR_BUFFER                 1         // Store messages into linear buffer
#define NORMAL_TRAFFIC                0         // Normal CAN traffic
#define CORRUPT_TRAFFIC               1         // Corrupted CAN traffic
#define SD_CHIP_SELECT                10        // Chip select pin for SD card
#define TIMESTAMP_SIZE                40        // Size of timestamp string
#define MIN_CORRUPT_TRAFFIC_READINGS  90000     // Amount of corrupt CAN messages that will be recorded after each UDS message


/* GLOBAL VARIABLES */
circular_buffer_t cb;                 // Normal CAN traffic circular buffer
SdFat sd;                             // SD Card object
char fileTimestamp[TIMESTAMP_SIZE];   // Timestamp for each file saved to SD card, this marks the start time of the program
uint8_t readType;                     // Type of storage used to store messages
uint8_t networkStatus;                // Current status of CAN traffic (Normal or Corrupt) that decides how the data should be stored
uint32_t corruptMsgCount;             // Number of messages since UDS message was monitored
uint32_t fileNumber;                  // Number cooresponding to each UDS attack instance
uint32_t numUDSMessages;              // Number of UDS messages in a single attack
bool errorStatus;                     // System error status


/** Creates a detailed timestamp
 *  This will be used to create the header timestamp on files saved the the SD Card
 *  @param *timestamp String to be populated with the timestamp
 */
void create_file_timestamp(char *timestamp, size_t strLen)
{
  time_t t = now();
  uint16_t Year = year(t);
  uint16_t Month = month(t);
  uint16_t Day = day(t);
  uint16_t Hour = hour(t);
  uint16_t Minute = minute(t);
  uint16_t Second = second(t);
  sprintf(timestamp, "%02d/%02d/%04d  %02d:%02d:%02d", Month, Day, Year, Hour, Minute, Second);
}


/** Dumps all circular buffer data to a new file on the SD Card
 *  @param *cb Circular buffer struct to be dumped to SD
 *  @return messageCount Number of messages read from circular buffer
 */
int circular_buffer_dump(circular_buffer_t *cb)
{
  can_message_t *readStart = NULL;
  can_message_t *readEnd = NULL;
  can_message_t *currentMessage = NULL;
  uint16_t messageCount = 0;
  SdFile cbFile;

  char fileName[30];
  sprintf(fileName, "Before_UDS_Attack_%lu.txt", fileNumber);
  
  configure_file(fileName, &cbFile);

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
    /*
    delay(5);
    Serial.print("CIRCULAR BUFFER MESSAGE #");
    Serial.print(count, DEC);
    Serial.print(" ");
    serial_print_can_message(currentMessage);
    */
    cbFile.print("MSG: ");
    cbFile.print(" ");
    cbFile.print(messageCount, DEC);
    cbFile.print(" ");
    file_print_message(currentMessage, &cbFile);
    currentMessage++;
    messageCount++;
    if(currentMessage == cb->bufferEnd)
    {
      currentMessage = cb->bufferStart;
    }
  }
  cbFile.close();
//  readFile(fileName, &cbFile);
  return messageCount;
}


/** Dumps all linear buffer data to an open file on the SD Card
 *  @param *lb Linear buffer struct to be dumped to SD
 *  @param *lbFile File to be written to
 *  @return messageCount Number of messages read from linear buffer
 */
int linear_buffer_dump(linear_buffer_t *lb, SdFile *lbFile)
{
  uint16_t messageCount = 0;
  can_message_t *currentMessage = lb->bufferStart;
  
  // iterate through linear buffer until we reach the end of the data
  while ((messageCount == 0) || (currentMessage != lb->bufferEnd))
  {
   /*
    lbFile.print("MSG: ");
    lbFile.print(" ");
    lbFile.print(messageCount, DEC);
    lbFile.print(" ");*/
    file_print_message(currentMessage, lbFile);
    currentMessage++;
    messageCount++;
  }
  return messageCount;
}


/** Sets parameters during a state change
 *  @param newReadType New 
 */
void changeState(int newReadType, int newNetworkStatus)
{
  readType = newReadType;
  networkStatus = newNetworkStatus;
  switch (newReadType)
  {
    case LINEAR_BUFFER:
    {
      Serial.println("Changing State: Circular -> Linear");
      numUDSMessages = 0;
      corruptMsgCount = 0;
      break;
    }
    case CIRCULAR_BUFFER:
    {
      Serial.println("Changing State: Linear -> Circular");
      circular_buffer_reinit(&cb);
      fileNumber++;
      break;
    }
    default:
    {
      break;
    }
  }
}

void setup(void)
{
  Serial.begin(115200);

  /* CAN Network Configuration */
  FLEXCAN_config_t canConfig;
  can_config_init(&canConfig);
  FLEXCAN_init(canConfig);

  /* Circular Buffer Configuration */
  circular_buffer_init(&cb, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));

  /* File Writing Configuration */
  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) 
  {
    Serial.println("Failed to init SD Card");
  }
  deleteAllFiles(&sd);
  create_file_timestamp(fileTimestamp, TIMESTAMP_SIZE);
  readType = CIRCULAR_BUFFER;
  networkStatus = NORMAL_TRAFFIC;

  /* Counters */
  corruptMsgCount = 0;
  fileNumber = 1;
  numUDSMessages = 0;
}


void loop(void)
{
    FLEXCAN_frame_t newFrame;

    switch (readType)
    { 
      case CIRCULAR_BUFFER:
      {
        if (generate_frame(&newFrame, 0, 8192)) // (can_fifo_read(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          transpose_can_message(&newMessage, &newFrame);

          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            circular_buffer_dump(&cb);
            Serial.print("  Attack found: ");
            serial_print_can_message(&newMessage);
            changeState(LINEAR_BUFFER, CORRUPT_TRAFFIC);
          }
          else
          {
            circular_buffer_push(&cb, &newMessage);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }

      case LINEAR_BUFFER:
      {
        if (generate_frame(&newFrame, 0, 50000)) // (can_fifo_read(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          transpose_can_message(&newMessage, &newFrame);
          corruptMsgCount++;
          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            Serial.print("  Another UDS Message found: ");
            Serial.print(newMessage.id, HEX);
            Serial.println(" (Extending read time)");
            corruptMsgCount = 0;
            numUDSMessages++;
          }
          else if (corruptMsgCount >= MIN_CORRUPT_TRAFFIC_READINGS)
          {
            // write number of UDS messages to file here
            Serial.println("  Done recording UDS traffic");
            changeState(CIRCULAR_BUFFER, CORRUPT_TRAFFIC);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }
      default:
      {
        break;
      }
    }
}

