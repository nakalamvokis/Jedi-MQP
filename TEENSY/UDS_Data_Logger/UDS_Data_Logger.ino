#include <DS1307RTC.h>

/*
  * File Name: UDS_Data_Logger.ino
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
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

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY      1800      // Maximum capacity of the circular buffer, equates to ~1.2 seconds of CAN data
#define LINEAR_BUFFER_CAPACITY        512       // Maximum capacity of the linear buffer
#define UDS_ID                        0x7E8     // Arbitration ID of all UDS messages sent to the vehicle
#define TEST_PACKET_TRANSFER_DELAY    1         // Packet simulation send delay time
#define SD_CHIP_SELECT                10        // Chip select pin for SD card
#define TIMESTAMP_SIZE                40        // Size of timestamp string
#define MIN_CORRUPT_TRAFFIC_READINGS  90000     // Amount of corrupt CAN messages that will be recorded after each UDS message
#define FILE_NAME_SIZE                50        // Maximum size of file names

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
      Serial.println("Changing State: Circular -> Linear");
      OpenNewLBFile(&g_LbFile, g_Timestamp, FILE_NAME_SIZE, g_Model.fileNumber);
      g_Model.numUDSMessages = 1;
      g_Model.corruptMsgCount = 1;
      break;
    }
    case eREAD_CIRCULAR_BUFFER:
    {
      Serial.println("Changing State: Linear -> Circular");
      g_Model.fileNumber++;
      break;
    }
    default:
    {
      break;
    }
  }
}

/** Opens and configures a new circular buffer file
 *  @param file SD file object
 *  @param directory Directory of the new data file
 *  @param nameSize Size of the file name
 *  @param fileNumber File number to be appended to the end of the file name
 */
void OpenNewCBFile(SdFile *file, char *directory, size_t nameSize, uint32_t fileNumber)
{
  char fileName[nameSize];
  snprintf(fileName, nameSize, "%s/Before_UDS_Attack_%lu.txt", directory, fileNumber);
  ConfigureFile(fileName, file); 
}

/** Opens and configures a new linear buffer data file
 *  @param file SD file object
 *  @param directory Directory of the new file
 *  @param nameSize Size of the file name
 *  @param fileNumber File number to be appended to the end of the file name
 */
void OpenNewLBFile(SdFile *file, char *directory, size_t nameSize, uint32_t fileNumber)
{
  char fileName[nameSize];
  snprintf(fileName, nameSize, "%s/After_UDS_Attack_%lu.txt", directory, fileNumber);
  ConfigureFile(fileName, file); 
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
  setSyncProvider(RTC.get);
  if(timeStatus()!= timeSet)
  {
    delay(1000);
    Serial.println("Unable to sync with the RTC");
    delay(1000);
  }

  /* File Writing Configuration */
  SdInit(&g_SD, SD_CHIP_SELECT);
  DeleteAllFiles(&g_SD);
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
            // Write contents of circular buffer to a new file
            OpenNewCBFile(&g_CbFile, g_Timestamp, FILE_NAME_SIZE, g_Model.fileNumber);
            CircularBufferDumpToFile(&g_CB, &g_CbFile);
            g_CbFile.close();
            
            Serial.print("  Attack found: ");
            SerialPrintCanMessage(&newMessage);
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
          if (g_LB.isFull)
          {
            LinearBufferDumpToFile(&g_LB, &g_LbFile);
          }
          LinearBufferPush(&g_LB, &newMessage);
          g_Model.corruptMsgCount++;
          g_Model.totalMsgCount++;
          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            Serial.print("  Another UDS Message found: ");
            Serial.print(newMessage.id, HEX);
            Serial.println(" (Extending read time)");
            g_Model.corruptMsgCount = 0;
            g_Model.numUDSMessages++;
          }
          else if (g_Model.corruptMsgCount >= MIN_CORRUPT_TRAFFIC_READINGS)
          {
            LinearBufferDumpToFile(&g_LB, &g_LbFile);
            // Write UDS message count for this attack to the "after attack" file
            char UDSMsgCountString[50];
            sprintf(UDSMsgCountString, "UDS Messages Recorded: %lu", g_Model.numUDSMessages);
            FilePrintString(UDSMsgCountString, &g_LbFile);
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

