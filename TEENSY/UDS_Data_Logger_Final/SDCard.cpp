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
  SdFile::dateTimeCallback(dateTime);
  if (!sd->begin(chipSelect, SPI_FULL_SPEED)) 
  {
    HandleError(eERR_SD_FAILED_INIT);
    status = false;
  }
  return status;
}

/** Sets the time for the SD card callback function
 *  @param *date Compressed date value
 *  @param &time Compressed time value
 */
void dateTime(uint16_t *date, uint16_t *time) 
{
  time_t t = now();
  *date = FAT_DATE(year(t), month(t), day(t)); // FAT_DATE formats the date fields
  *time = FAT_TIME(hour(t), minute(t), second(t)); // FAT_TIME formats the time fields
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

/** Prints the file name and data column headers to a file
 *  @param *file File to be configured
 *  @param *fileName Name of the file
 */
void ConfigureDataFile(SdFile *file, char* fileName)
{
  file->println(fileName);
  file->println();
  file->print("TIMESTAMP (MS)");
  file->print("\t");
  file->print("ID");
  file->print("\t");
  file->print("DATA");
  file->println();
}

/** Opens and configures a new data file for write
 *  @param *file SD file object
 *  @param *filePath Full path of the new file
 *  @param *fileName Name of the new file
 *  @return Status of the file open
 */
bool OpenNewDataFile(SdFile *file, char *filePath, char *fileName)
{
  bool status = true;
  
  if (!file->open(filePath, O_RDWR | O_CREAT | O_AT_END)) 
  {
    HandleError(eERR_SD_FAILED_FILE_OPEN_FOR_WRITE);
    status = false;
  }
  
  ConfigureDataFile(file, fileName);
  return status;
}

/** Opens a previously created data file for write
 *  @param *file SD file object
 *  @param *filePath Path of the file on the SD card
 *  @return Status of the file open
 */
bool OpenDataFile(SdFile *file, char *filePath)
{
  bool status = true;
  if (!file->open(filePath, O_RDWR | O_CREAT | O_AT_END)) 
  {
    HandleError(eERR_SD_FAILED_FILE_OPEN_FOR_WRITE);
    status = false;
  }
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

