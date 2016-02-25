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
#define UDS_ID                        0x002    // Arbitration ID of all UDS messages sent to the vehicle
#define CIRCULAR_BUFFER_CAPACITY      1800      // Maximum capacity of the circular buffer, equates to ~1.2 seconds of CAN data
#define LINEAR_BUFFER_CAPACITY        256       // Maximum capacity of the linear buffer
#define MIN_CORRUPT_TRAFFIC_READINGS  90000     // Amount of corrupt CAN messages that will be recorded after each UDS message

#define DIAG 1

/* CONSTANTS */
const char g_CbFileName[FILE_NAME_SIZE] = "Before_UDS_Attack_";
const char g_LbFileName[FILE_NAME_SIZE] = "After_UDS_Attack_";

/* GLOBAL VARIABLES */
circular_buffer_t g_CB;                   // Circular buffer
linear_buffer_t g_LB;                     // Linear buffer
SdFat g_SD;                               // SD Card object
model_t g_Model;                          // System model
char g_Timestamp[TIMESTAMP_SIZE];         // Timestamp for each file saved to SD card, this marks the start time of the program
SdFile g_CurrentCbFile;                   // File object of file traffic is currently being written to
char g_currentCbFilePath[FILE_PATH_SIZE]; // Path of file traffic is currently being written to
char g_currentCbFileName[FILE_NAME_SIZE]; // Name of file traffic is currently being written to
SdFile g_CurrentLbFile;                   // File object of file traffic is currently being written to
char g_currentLbFilePath[FILE_PATH_SIZE]; // Path of file traffic is currently being written to
char g_currentLbFileName[FILE_NAME_SIZE]; // Name of file traffic is currently being written to


bool Cb_write = false;
bool Lb_write = false;


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
      SetFileNameAndPath(g_currentLbFilePath, g_currentLbFileName, g_Timestamp, g_LbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
      OpenNewDataFile(&g_CurrentLbFile, g_currentLbFilePath, g_currentLbFileName);
      g_CurrentLbFile.close();
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

void can_fifo_callback(uint8_t x)
{
  if(FLEXCAN_fifo_avalible())
  {
    FLEXCAN_frame_t newFrame;
    can_message_t newMessage;
    FLEXCAN_fifo_read(&newFrame);
    TransposeCanMessage(&newMessage, &newFrame);


  //  Serial.println(newFrame.id, HEX);
    
    if(newMessage.id == UDS_ID)
    {
      #ifdef DIAG
      delay(1000);
      Serial.println("Found UDS");
      delay(1000);
      #endif
    }
    
    
    switch (g_Model.readType)
    { 
      case eREAD_CIRCULAR_BUFFER:
      {
        if (newMessage.id == UDS_ID) // UDS message detected, an attack was launched
        {
          if (!g_SD.exists(g_Timestamp)) // create a new directory after the first attack starts
          {
            MakeDirectory(g_Timestamp, &g_SD);
          }
          Cb_write = true;
          LinearBufferPush(&g_LB, &newMessage);
          ChangeState(eREAD_LINEAR_BUFFER, eSTATE_CORRUPT_TRAFFIC);
     
          #ifdef DIAG
              Serial.print("  Attack found: ");
              SerialPrintCanMessage(&newMessage);
           #endif
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
          Lb_write = true;
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
  
  if (Cb_write == true)
  {
    #ifdef DIAG
      Serial.println("Found attack - dumping circular buffer to SD card");
    #endif
    
    SetFileNameAndPath(g_currentCbFilePath, g_currentCbFileName, g_Timestamp, g_CbFileName, g_Model.fileNumber, FILE_NAME_SIZE, FILE_PATH_SIZE);
    OpenNewDataFile(&g_CurrentCbFile, g_currentCbFilePath, g_currentCbFileName);
    CircularBufferDumpToFile(&g_CB, &g_CurrentCbFile);
    g_CurrentCbFile.close();
    Cb_write = false;
  }
     
  else if (g_LB.isFull)
  {
    #ifdef DIAG
      Serial.println("Linear buffer full - dumping linear buffer to SD card");
    #endif
    
    OpenDataFile(&g_CurrentLbFile, g_currentLbFilePath);
    LinearBufferDumpToFile(&g_LB, &g_CurrentLbFile);
    g_CurrentLbFile.close();
  }
        
  else if (Lb_write)
  {
    #ifdef DIAG
      Serial.println("UDS Message end - dumping linear buffer to SD card");
    #endif
    
    OpenDataFile(&g_CurrentLbFile, g_currentLbFilePath);
    LinearBufferDumpToFile(&g_LB, &g_CurrentLbFile);
    char UDSMsgCountString[50];
    sprintf(UDSMsgCountString, "\nUDS Messages Recorded: %lu", g_Model.numUDSMessages);
    g_CurrentLbFile.println(UDSMsgCountString);
    g_CurrentLbFile.close();
    Lb_write = false;
  }
}

