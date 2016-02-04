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
const char g_CbFileName[FILE_NAME_SIZE] = "Before_UDS_Attack_";
const char g_LbFileName[FILE_NAME_SIZE] = "After_UDS_Attack_";

/* GLOBAL VARIABLES */
circular_buffer_t g_CB;                 // Circular buffer
linear_buffer_t g_LB;                   // Linear buffer
SdFat g_SD;                             // SD Card object
SdFile g_CbFile;                        // Circular buffer file object
SdFile g_LbFile;                        // Linear buffer file object
model_t g_Model;                        // System model
char g_Timestamp[TIMESTAMP_SIZE];       // Timestamp for each file saved to SD card, this marks the start time of the program
char g_currentFilePath[FILE_PATH_SIZE]; // Path of file traffic will be written to
char g_currentFileName[FILE_NAME_SIZE]; // Name of file traffic will be written to

/** Sets the file name and path for a new data file
 *  This function is used to set these parameters before opening a new file
 *  @param *filePath Full path of the file (to be set)
 *  @param *fileName Name of the file (to be set)
 *  @param *directory Directory of the new file
 *  @param *fileTitle Name of new file
 *  @param fileNumber File number to be appended to the end of the file name
 *  @param nameSize Size of the file name
 *  @param pathSize Size of the file path
 */
void SetFileNameAndPath(char *filePath, char *fileName, char *directory, const char *fileTitle, uint32_t fileNumber, size_t nameSize, size_t pathSize)
{
  snprintf(fileName, nameSize, "%s%lu.txt", fileTitle, fileNumber);
  snprintf(filePath, pathSize, "%s/%s", directory, fileName);
}

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
      SetFileNameAndPath(g_currentFilePath, g_currentFileName, g_Timestamp, g_LbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
      OpenNewDataFile(&g_LbFile, g_currentFilePath, g_currentFileName);
      g_LbFile.close();
      break;
    }
    case eREAD_CIRCULAR_BUFFER:
    {
      #ifdef DIAG
        Serial.println("Changing State: Linear -> Circular");
      #endif
      
      g_Model.fileNumber++;
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
            SetFileNameAndPath(g_currentFilePath, g_currentFileName, g_Timestamp, g_CbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
            OpenNewDataFile(&g_CbFile, g_currentFilePath, g_currentFileName);
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
            OpenDataFile(&g_LbFile, g_currentFilePath);
            LinearBufferDumpToFile(&g_LB, &g_LbFile);
            g_LbFile.close();
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
            OpenDataFile(&g_LbFile, g_currentFilePath);
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

