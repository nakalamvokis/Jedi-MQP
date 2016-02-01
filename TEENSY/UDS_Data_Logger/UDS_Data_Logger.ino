/*
  * @file UDS_Data_Logger.ino
  * @author Nicholas Kalamvokis
  * @date 1/21/2016
  *
  *
*/

/* INCLUDES */
#include <SPI.h>
#include <Wire.h>
#include <can.h>
#include <SdFat.h>
#include <Time.h>
#include <DS1307RTC.h>
#include "UDSDataLogger.h"

/* DEFINES */
#define CIRCULAR_BUFFER_CAPACITY      1800      // Maximum capacity of the circular buffer, equates to ~1.2 seconds of CAN data
#define LINEAR_BUFFER_CAPACITY        256       // Maximum capacity of the linear buffer
#define MIN_CORRUPT_TRAFFIC_READINGS  90000     // Amount of corrupt CAN messages that will be recorded after each UDS message

#define DIAG 1

/* CONSTANTS */
const char cbFileName[FILE_NAME_SIZE] = "Before_UDS_Attack_";
const char lbFileName[FILE_NAME_SIZE] = "After_UDS_Attack_";

/* GLOBAL VARIABLES */
circular_buffer_t g_CB;             // Circular buffer
linear_buffer_t g_LB;               // Linear buffer
SdFat g_SD;                         // SD Card object
SdFile g_CbFile;                    // Circular buffer file object
SdFile g_LbFile;                    // Linear buffer file object
model_t g_Model;                    // System model
char g_Timestamp[TIMESTAMP_SIZE];   // Timestamp for each file saved to SD card, this marks the start time of the program

/** Sets parameters during a state change
 *  @param newReadType New 
 */
void ChangeState(ReadType_e newReadType, NetworkState_e newNetworkState)
{
  g_Model.readType = newReadType;
  g_Model.networkState = newNetworkState;
  
  switch (newReadType)
  {
    case eREAD_LINEAR_BUFFER:
    {
      #ifdef DIAG
        Serial.println("Changing State: Circular -> Linear");
      #endif
      
      g_Model.numUDSMessages = 1;
      g_Model.corruptMsgCount = 1;
      OpenNewDataFile(&g_LbFile, g_Timestamp, lbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
      break;
    }
    case eREAD_CIRCULAR_BUFFER:
    {
      #ifdef DIAG
        Serial.println("Changing State: Linear -> Circular");
      #endif
      
      g_Model.fileNumber++;
      OpenNewDataFile(&g_CbFile, g_Timestamp, cbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
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
  
  /* Model Configuration */
  g_Model.readType = eREAD_CIRCULAR_BUFFER;
  g_Model.networkState = eSTATE_NORMAL_TRAFFIC;
  g_Model.corruptMsgCount = 0;
  g_Model.fileNumber = 1;
  g_Model.numUDSMessages = 0;
  g_Model.totalMsgCount = 0;

  /* CAN Network Configuration */
  FLEXCAN_config_t canConfig;
  CanConfigInit(&canConfig);
  FLEXCAN_init(canConfig);

  /* Buffer Configuration */
  CircularBufferInit(&g_CB, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));
  LinearBufferInit(&g_LB, LINEAR_BUFFER_CAPACITY, sizeof(can_message_t));

  /* Timing Configuration */
  RTCInit();

  /* File Writing Configuration */
  SdInit(&g_SD, SD_CHIP_SELECT);
  SetTimestamp(g_Timestamp, TIMESTAMP_SIZE);
  MakeDirectory(g_Timestamp, &g_SD);
  OpenNewDataFile(&g_CbFile, g_Timestamp, cbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
}

void loop(void)
{
    CheckStatus(&g_SD);
    FLEXCAN_frame_t newFrame;
    switch (g_Model.readType)
    { 
      case eREAD_CIRCULAR_BUFFER:
      {
        if (GenerateFrame(&newFrame, 0, 8192)) // (CanFifoRead(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          TransposeCanMessage(&newMessage, &newFrame);
          g_Model.totalMsgCount++;

          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            CircularBufferDumpToFile(&g_CB, &g_CbFile);
            g_CbFile.close();

            #ifdef DIAG
              Serial.print("  Attack found: ");
              SerialPrintCanMessage(&newMessage);
            #endif
            
            ChangeState(eREAD_LINEAR_BUFFER, eSTATE_CORRUPT_TRAFFIC);
            LinearBufferPush(&g_LB, &newMessage);
          }
          else
          {
            CircularBufferPush(&g_CB, &newMessage);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }

      case eREAD_LINEAR_BUFFER:
      {
        if (GenerateFrame(&newFrame, 0, 65535)) // (CanFifoRead(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          TransposeCanMessage(&newMessage, &newFrame);
          LinearBufferPush(&g_LB, &newMessage);
          g_Model.corruptMsgCount++;
          g_Model.totalMsgCount++;
          
          if (g_LB.isFull)
          {
            LinearBufferDumpToFile(&g_LB, &g_LbFile);
          }

          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            #ifdef DIAG
              Serial.print("  Another UDS Message found: ");
              Serial.print(newMessage.id, HEX);
              Serial.println(" (Extending read time)");
            #endif
            
            g_Model.corruptMsgCount = 0;
            g_Model.numUDSMessages++;
          }
          else if (g_Model.corruptMsgCount >= MIN_CORRUPT_TRAFFIC_READINGS)
          {
            LinearBufferDumpToFile(&g_LB, &g_LbFile);
            char UDSMsgCountString[50];
            sprintf(UDSMsgCountString, "UDS Messages Recorded: %lu", g_Model.numUDSMessages);
            g_LbFile.println(UDSMsgCountString);
            g_LbFile.close();
            
            ChangeState(eREAD_CIRCULAR_BUFFER, eSTATE_CORRUPT_TRAFFIC);
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

