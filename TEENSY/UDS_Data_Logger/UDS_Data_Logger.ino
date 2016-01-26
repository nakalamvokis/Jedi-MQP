/*
  * File Name: UDS_DATA_LOGGER.ino
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
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
#include "Errors.h"

/* FUNCTION PROTOTYPES */
void create_file_timestamp(char *timestamp, size_t strLen);
void changeState(int newReadType, int newNetworkStatus);

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY      1800      // Maximum capacity of the circular buffer, equates to ~1.2 seconds of CAN data
#define LINEAR_BUFFER_CAPACITY        1000      // Maximum capacity of the linear buffer
#define UDS_ID                        0x7E8     // Arbitration ID of all UDS messages sent to the vehicle
#define TEST_PACKET_TRANSFER_DELAY    1         // Packet simulation send delay time
#define CIRCULAR_BUFFER               0         // Store messages into circular buffer
#define LINEAR_BUFFER                 1         // Store messages into linear buffer
#define NORMAL_TRAFFIC                0         // Normal CAN traffic
#define CORRUPT_TRAFFIC               1         // Corrupted CAN traffic
#define SD_CHIP_SELECT                10        // Chip select pin for SD card
#define TIMESTAMP_SIZE                40        // Size of timestamp string
#define MIN_CORRUPT_TRAFFIC_READINGS  9000     // Amount of corrupt CAN messages that will be recorded after each UDS message


/* GLOBAL VARIABLES */
circular_buffer_t cb;                 // Circular buffer
linear_buffer_t lb;                   // Linear buffer
SdFat sd;                             // SD Card object
SdFile cbFile;                        // Circular buffer file object
SdFile lbFile;                        // Linear buffer file object
char fileTimestamp[TIMESTAMP_SIZE];   // Timestamp for each file saved to SD card, this marks the start time of the program
uint8_t readType;                     // Type of storage used to store messages
uint8_t networkStatus;                // Current status of CAN traffic (Normal or Corrupt) that decides how the data should be stored
uint32_t corruptMsgCount;             // Number of messages since UDS message was monitored
uint32_t fileNumber;                  // Number cooresponding to each UDS attack instance
uint32_t numUDSMessages;              // Number of UDS messages in a single attack
uint32_t totalMsgCount;               // Total number of messages recorded
bool errorStatus;                     // System error status


/** Creates a detailed timestamp
 *  This will be used to create the header timestamp on files saved the the SD Card
 *  @param *timestamp String to be populated with the timestamp
 */
void CreateFileTimestamp(char *timestamp, size_t strLen)
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


/** Sets parameters during a state change
 *  @param newReadType New 
 */
void ChangeState(int newReadType, int newNetworkStatus)
{
  readType = newReadType;
  networkStatus = newNetworkStatus;
  switch (newReadType)
  {
    case LINEAR_BUFFER:
    {
      Serial.println("Changing State: Circular -> Linear");
      numUDSMessages = 1;
      corruptMsgCount = 0;
      char fileName[30];
      sprintf(fileName, "After_UDS_Attack_%lu.txt", fileNumber);
      ConfigureFile(fileName, &lbFile);
      break;
    }
    case CIRCULAR_BUFFER:
    {
      Serial.println("Changing State: Linear -> Circular");
      CircularBufferReinit(&cb);
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
  CanConfigInit(&canConfig);
  FLEXCAN_init(canConfig);

  /* Buffer Configuration */
  CircularBufferInit(&cb, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));
  LinearBufferInit(&lb, LINEAR_BUFFER_CAPACITY, sizeof(can_message_t));

  /* File Writing Configuration */
  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) 
  {
    Serial.println("Failed to init SD Card");
  }
  DeleteAllFiles(&sd);
  CreateFileTimestamp(fileTimestamp, TIMESTAMP_SIZE);
  readType = CIRCULAR_BUFFER;
  networkStatus = NORMAL_TRAFFIC;

  /* Counters */
  corruptMsgCount = 0;
  fileNumber = 1;
  numUDSMessages = 0;
  totalMsgCount = 0;
}


void loop(void)
{
    FLEXCAN_frame_t newFrame;

    switch (readType)
    { 
      case CIRCULAR_BUFFER:
      {
        if (GenerateFrame(&newFrame, 0, 8192)) // (CanFifoRead(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          TransposeCanMessage(&newMessage, &newFrame);
          totalMsgCount++;

          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            // Write contents of circular buffer to a new file
            char fileName[30];
            sprintf(fileName, "Before_UDS_Attack_%lu.txt", fileNumber);
            ConfigureFile(fileName, &cbFile);
            CircularBufferDumpToFile(&cb, &cbFile);
            cbFile.close();

            
            Serial.print("  Attack found: ");
            SerialPrintCanMessage(&newMessage);
            ChangeState(LINEAR_BUFFER, CORRUPT_TRAFFIC);
            LinearBufferPush(&lb, &newMessage);
          }
          else
          {
            CircularBufferPush(&cb, &newMessage);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }

      case LINEAR_BUFFER:
      {
        if (GenerateFrame(&newFrame, 0, 50000)) // (CanFifoRead(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          TransposeCanMessage(&newMessage, &newFrame);
          if (lb.isFull == true)
          {
            LinearBufferDumpToFile(&lb, &lbFile);
            LinearBufferReinit(&lb);
          }
          LinearBufferPush(&lb, &newMessage);
          corruptMsgCount++;
          totalMsgCount++;
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
            // Write UDS message count for this attack to the "after attack" file
            char UDSMsgCountString[30];
            sprintf(UDSMsgCountString, "UDS Messages Recorded: %lu", numUDSMessages);
            FilePrintString(UDSMsgCountString, &lbFile);
            lbFile.close();
            
            Serial.println("  Done recording UDS traffic");
            ChangeState(CIRCULAR_BUFFER, CORRUPT_TRAFFIC);
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

