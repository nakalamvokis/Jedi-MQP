/*
  * File Name: UDSDataLogger.h
  * Date: 1/26/2016
  * Author: Nicholas Kalamvokis
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
void ChangeState(ReadType_e newReadType, NetworkState_e newNetworkState);
void OpenNewCBFile(SdFile *file, char *directory, size_t nameSize, uint32_t fileNumber);
void OpenNewLBFile(SdFile *file, char *directory, size_t nameSize, uint32_t fileNumber);

#endif // UDSDATALOGGER_H

