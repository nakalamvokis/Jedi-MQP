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
#include <SPI.h>
#include <can.h>
#include <SdFat.h>
#include <Time.h>
#include "CircularBuffer.h"
#include "LinearBuffer.h"
#include "CANMessage.h"
#include "SDCard.h"
#include "Errors.h"

/* STRUCTS */
typedef struct {
  uint8_t readType;                     // Type of storage used to store messages
  uint8_t networkStatus;                // Current status of CAN traffic (Normal or Corrupt) that decides how the data should be stored
  uint32_t corruptMsgCount;             // Number of messages since UDS message was monitored
  uint32_t fileNumber;                  // Number cooresponding to each UDS attack instance
  uint32_t numUDSMessages;              // Number of UDS messages in a single attack
  uint32_t totalMsgCount;               // Total number of messages recorded
} model_t;

/* FUNCTION PROTOTYPES */
void SetTimestamp(char *timestamp, size_t strLen);
void ChangeState(int newReadType, int newNetworkStatus);

#endif // UDSDATALOGGER_H

