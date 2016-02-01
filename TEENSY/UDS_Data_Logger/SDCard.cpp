/*
  * @file SDCard.cpp
  * @author Nicholas Kalamvokis
  * @date 1/21/2016
  *
  * 
*/

#include "SDCard.h"

/** Initializes SD Card
 *  @param *sd SD card object
 *  @param chipSelect Chip select pin for SD card reader
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
 *  @param *dirName Name of new directory
 *  @param *sd SD card object
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
 *  @param *filePath Full path of file
 *  @param *fileName Name of file
 *  @param *file File to be configured
 */
bool ConfigureDataFile(char *filePath, char* fileName, SdFile *file)
{
  bool status = true;
  
  if (!file->open(filePath, O_RDWR | O_CREAT | O_AT_END)) 
  {
    HandleError(eERR_SD_FAILED_FILE_OPEN_FOR_WRITE);
    status = false;
  }

  SetFileCreateTime(file);
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

/** Opens and configures a new data file for write
 *  @param *file SD file object
 *  @param *directory Directory of the new data file
 *  @param *fileName Name of new file
 *  @param fileNumber File number to be appended to the end of the file name
 *  @param pathSize Size of the file name
 */
void OpenNewDataFile(SdFile *file, char *directory, const char *fileName, uint32_t fileNumber, size_t nameSize, size_t pathSize)
{
  char filePath[pathSize];
  char fullFileName[nameSize];
  snprintf(fullFileName, nameSize, "%s%lu.txt", fileName, fileNumber);
  snprintf(filePath, pathSize, "%s/%s", directory, fullFileName);
  ConfigureDataFile(filePath, fullFileName, file); 
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
    int data;
    SetFileAccessTime(file);
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
 *  @param *sd SD card object
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
void FileWriteMessage(can_message_t *message, SdFile *file)
{
  uint8_t currentData = 0;
  file->print(message->timestamp, DEC);
  file->print("\t\t");
  file->print(message->id, HEX);
  file->print("\t");
  for (currentData = 0; currentData < message->len; currentData++)
  {
    file->print(message->data[currentData], HEX);
    file->print(" ");
  }
  file->println();
}

/** Checks the status of the SD card
 *  @param *sd SD card object
 *  @return Status of SD card communications 
 */
bool CheckStatus(SdFat *sd)
{
  bool status = true;
  uint32_t ocr;
  if (!sd->card()->readOCR(&ocr)) 
  {
    HandleError(eERR_SD_LOST_COMMUNICATIONS);
    status = false;
  }
  return status;
}

/** Sets the creation date of a file to the current time
 *  @param *file File to be edited
 *  @return Status of the timestamp edit
 */
bool SetFileCreateTime(SdFile *file)
{
  bool status = true;
  time_t t = now();
  
  if (!file->timestamp(T_CREATE, year(t), month(t), day(t), hour(t), minute(t), second(t))) 
  {
    HandleError(eERR_SD_FAILED_TO_EDIT_FILE_TIMESTAMP);
    status = false;
  }
  return status;
}

/** Sets the edit date of a file to the current time
 *  @param *file File to be edited
 *  @return Status of the timestamp edit
 */
bool SetFileEditTime(SdFile *file)
{
  bool status = true;
  time_t t = now();
  
  if (!file->timestamp(T_WRITE, year(t), month(t), day(t), hour(t), minute(t), second(t))) 
  {
    HandleError(eERR_SD_FAILED_TO_EDIT_FILE_TIMESTAMP);
    status = false;
  }
  SetFileAccessTime(file); // Access in included in write
  return status;
}

/** Sets the access date of a file to the current time
 *  @param *file File to be edited
 *  @return Status of the timestamp edit
 */
bool SetFileAccessTime(SdFile *file)
{
  bool status = true;
  time_t t = now();
  
  if (!file->timestamp(T_ACCESS, year(t), month(t), day(t), hour(t), minute(t), second(t))) 
  {
    HandleError(eERR_SD_FAILED_TO_EDIT_FILE_TIMESTAMP);
    status = false;
  }
  return status;
}

