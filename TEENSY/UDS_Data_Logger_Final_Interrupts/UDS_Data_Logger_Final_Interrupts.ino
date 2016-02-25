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
#define UDS_ID                        0x7E8    // Arbitration ID of all UDS messages sent to the vehicle
#define CIRCULAR_BUFFER_CAPACITY      1800      // Maximum capacity of the circular buffer, equates to ~1.2 seconds of CAN data
#define LINEAR_BUFFER_CAPACITY        256       // Maximum capacity of the linear buffer
#define MIN_CORRUPT_TRAFFIC_READINGS  90000     // Amount of corrupt CAN messages that will be recorded after each UDS message

//#define DIAG 1
#define PRINT 1

/* CONSTANTS */
const char g_CbFileName[FILE_NAME_SIZE] = "Before_UDS_Attack_";
const char g_LbFileName[FILE_NAME_SIZE] = "After_UDS_Attack_";

/* GLOBAL VARIABLES */
circular_buffer_t g_CB;                   // Circular buffer
linear_buffer_t g_LB;                     // Linear buffer
SdFat g_SD;                               // SD Card object
model_t g_Model;                          // System model
char g_Timestamp[TIMESTAMP_SIZE];         // Timestamp for each file saved to SD card, this marks the start time of the program
SdFile g_CurrentFile;                     // File object of file traffic is currently being written to
char g_currentFilePath[FILE_PATH_SIZE];   // Path of file traffic is currently being written to
char g_currentFileName[FILE_NAME_SIZE];   // Name of file traffic is currently being written to


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
      OpenNewDataFile(&g_CurrentFile, g_currentFilePath, g_currentFileName);
      g_CurrentFile.close();
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

/** Callback function for the CAN hardware fifo queue
 *  This callback is used to avoid the use of polling and therefore, increase CAN read speeds
 */
void can_fifo_callback(uint8_t x)
{
  CheckStatus(&g_SD);
  if(FLEXCAN_fifo_avalible())
  {
    FLEXCAN_frame_t newFrame;
    can_message_t newMessage;
    FLEXCAN_fifo_read(&newFrame);
    TransposeCanMessage(&newMessage, &newFrame);

    #ifdef PRINT
      delay(100);
      Serial.println(newMessage.id, HEX);
      delay(1000);
      if (newMessage.id == UDS_ID)
      {
        Serial.println("Found UDS");
      }
    #endif
    
    switch (g_Model.readType)
    { 
      case eREAD_CIRCULAR_BUFFER:
      {
        if (newMessage.id == UDS_ID) // UDS message detected, an attack was launched
        {
          #ifdef DIAG
            Serial.println("Found attack - dumping circular buffer to SD card");
            SerialPrintCanMessage(&newMessage);
          #endif
          
          if (!g_SD.exists(g_Timestamp)) // create a new directory after the first attack starts
          {
            MakeDirectory(g_Timestamp, &g_SD);
          }

          SetFileNameAndPath(g_currentFilePath, g_currentFileName, g_Timestamp, g_CbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
          OpenNewDataFile(&g_CurrentFile, g_currentFilePath, g_currentFileName);
          CircularBufferDumpToFile(&g_CB, &g_CurrentFile);
          g_CurrentFile.close();
          
          LinearBufferPush(&g_LB, &newMessage);
          ChangeState(eREAD_LINEAR_BUFFER, eSTATE_CORRUPT_TRAFFIC);
        }
        else
        {
          CircularBufferPush(&g_CB, &newMessage);
        }
        g_Model.totalMsgCount++;
        break;
      }

      case eREAD_LINEAR_BUFFER:
      {
        LinearBufferPush(&g_LB, &newMessage);
        g_Model.corruptMsgCount++;
        g_Model.totalMsgCount++;

        if (g_LB.isFull)
        {
          #ifdef DIAG
            Serial.println("Linear buffer full - dumping linear buffer to SD card");
          #endif
    
          OpenDataFile(&g_CurrentFile, g_currentFilePath);
          LinearBufferDumpToFile(&g_LB, &g_CurrentFile);
          g_CurrentFile.close();
        }
        
        if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
        {
          g_Model.corruptMsgCount = 0;
          g_Model.numUDSMessages++;
          
          #ifdef DIAG
            Serial.print("  Another UDS Message found: ");
            Serial.print(newMessage.id, HEX);
            Serial.println(" (Extending read time)");
          #endif
        }
        else if (g_Model.corruptMsgCount >= MIN_CORRUPT_TRAFFIC_READINGS)
        {
          #ifdef DIAG
            Serial.println("UDS Message end - dumping linear buffer to SD card");
          #endif
    
          OpenDataFile(&g_CurrentFile, g_currentFilePath);
          LinearBufferDumpToFile(&g_LB, &g_CurrentFile);
          char UDSMsgCountString[50];
          sprintf(UDSMsgCountString, "\nUDS Messages Recorded: %lu", g_Model.numUDSMessages);
          g_CurrentFile.println(UDSMsgCountString);
          g_CurrentFile.close();
          ChangeState(eREAD_CIRCULAR_BUFFER, eSTATE_CORRUPT_TRAFFIC);
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
   return;
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

  /* Buffer Configuration */
  CircularBufferInit(&g_CB, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));
  LinearBufferInit(&g_LB, LINEAR_BUFFER_CAPACITY, sizeof(can_message_t));

  /* Timing Configuration */
  RTCInit();

  /* File Writing Configuration */
  SetTimestamp(g_Timestamp, TIMESTAMP_SIZE);
  SdInit(&g_SD, SD_CHIP_SELECT);

  /* CAN Network Configuration */
  FLEXCAN_config_t canConfig;
  CanConfigInit(&canConfig);
  FLEXCAN_init(canConfig);
  FLEXCAN_fifo_reg_callback(can_fifo_callback);
}

void loop(void)
{
  CheckStatus(&g_SD);
  delay(10);
}

