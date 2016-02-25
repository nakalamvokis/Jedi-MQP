/*
  * @file UDSDataLogger.h
  * @author Nicholas Kalamvokis
  * @date 1/26/2016
  *
  * 
*/

#ifndef UDSDATALOGGER_H
#define UDSDATALOGGER_H

/* INCLUDES */
#include "CircularBuffer.h"
#include "LinearBuffer.h"
#include "CANMessage.h"
#include "SDCard.h"
#include "Errors.h"
#include "TimeModule.h"

/* DEFINES */
#define TEST_PACKET_TRANSFER_DELAY    1         // Packet simulation send delay time
#define SD_CHIP_SELECT                10        // Chip select pin for SD card
#define TIMESTAMP_SIZE                30        // Size of timestamp string
#define FILE_NAME_SIZE                30        // Size of file name string
#define FILE_PATH_SIZE                60        // Maximum size of file names

/* ENUMS */
enum NetworkState_e
{
  eSTATE_NONE = 0,
  eSTATE_NORMAL_TRAFFIC,
  eSTATE_CORRUPT_TRAFFIC
};

enum ReadType_e
{
  eREAD_NONE = 0,
  eREAD_CIRCULAR_BUFFER,
  eREAD_LINEAR_BUFFER
};

/* STRUCTS */
typedef struct {
  ReadType_e readType;                  // Type of storage used to store messages
  NetworkState_e networkState;          // Current status of CAN traffic (Normal or Corrupt) that decides how the data should be stored
  uint32_t corruptMsgCount;             // Number of messages since UDS message was monitored
  uint32_t fileNumber;                  // Number cooresponding to each UDS attack instance
  uint32_t numUDSMessages;              // Number of UDS messages in a single attack
  uint32_t totalMsgCount;               // Total number of messages recorded
} model_t;

/* FUNCTION PROTOTYPES */
void SetFileNameAndPath(char *filePath, char *fileName, char *directory, const char *fileTitle, uint32_t fileNumber, size_t nameSize, size_t pathSize);
void ChangeState(ReadType_e newReadType, NetworkState_e newNetworkState);
void can_fifo_callback(uint8_t x);

#endif // UDSDATALOGGER_H

