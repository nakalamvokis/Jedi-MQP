/*
  * File Name: SDCard.cpp
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#include "SDCard.h"

/** Initializes SD Card
 * @param sd SD card object
 * @param chipSelect Chip select pin for SD card reader
 */
bool SdInit(SdFat *sd, uint8_t chipSelect)
{
  bool status = true;
  if (!sd->begin(chipSelect, SPI_FULL_SPEED)) 
  {
    HandleError(eERR_SD_FAILED_INIT);
    status = false;
  }
  return status;
}

/** Makes a new directory on the SD Card
 * @param *dirName Name of new directory
 * @param *sd SD card object
 */
bool MakeDirectory(char *dirName, SdFat *sd)
{
  bool status = true;
  if (!sd->mkdir(dirName)) 
  {
    HandleError(eERR_SD_FAILED_TO_CREATE_DIRECTORY);
    status = false;
  } 
  return status;
}


/** Opens a new file and prints a timestamp header
 *  @param *fileName Name of file on SD card
 *  @param *file File to be configured
 */
bool ConfigureFile(char *fileName, SdFile *file)
{
  bool status = true;
  if (!file->open(fileName, O_RDWR | O_CREAT | O_AT_END)) 
  {
    HandleError(eERR_SD_FAILED_FILE_OPEN_FOR_WRITE);
    status = false;
  }
  file->println(fileName);
  file->println();
  file->print("TIMESTAMP (MS)");
  file->print("\t");
  file->print("ID");
  file->print("\t");
  file->print("DATA");
  file->println();
  return status;
}


/** Reads all contents of a file
 *  @param *fileName Name of file on SD card
 *  @param *file File to be read
 *  @return Whether or not the file could be opened
 */
bool ReadFile(char *fileName, SdFile *file)
{
  bool status = true;
  if (!file->open(fileName, O_READ)) 
  {
    HandleError(eERR_SD_FAILED_FILE_OPEN_FOR_READ);
    status = false;
  }
  else
  {
    int data; // changed from uint8_t to int -> check into this
    while ((data = file->read()) >= 0) 
    {
      delay(1);
      Serial.write(data);
    }
    file->close();
  }
  return status;
}


/** Deletes all files on the SD card
 *  @param sd SD card object
 *  @return Whether or not the rm was successful
 */
bool DeleteAllFiles(SdFat *sd)
{
  bool status = true;
  if (!sd->vwd()->rmRfStar())
  {
    HandleError(eERR_SD_FAILED_FILE_DELETE);
    status = false;
  }
  return status;
}


/** Prints a CAN message to a file
 *  @param *message CAN message to be printed to the file
 *  @param *file File to be printed to
 * 
 */
void FilePrintMessage(can_message_t *message, SdFile *file)
{
  uint8_t currentData = 0;
  file->print(message->timestamp, DEC);
  file->print("\t");
  file->print("\t");
  file->print(message->id, HEX);
  file->print("\t");
  for (currentData = 0; currentData < message->len; currentData++)
  {
    file->print(message->data[currentData], HEX);
    file->print(" ");
  }
  file->println();
}

/** Prints text to a file
 *  @param *text String to be printed to the file
 *  @param *file File to be printed to
 */
void FilePrintString(char *text, SdFile *file)
{
  file->println(text);
}

