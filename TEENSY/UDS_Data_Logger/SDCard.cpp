/*
  * File Name: SDCard.cpp
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#include "SDCard.h"

/** Opens a new file and prints a timestamp header
 *  @param *fileName Name of file on SD card
 *  @param *file File to be configured
 */
bool configure_file(char *fileName, SdFile *file)
{
  if (!file->open(fileName, O_RDWR | O_CREAT | O_AT_END)) 
  {
    Serial.println("Failed to open file for write");
    return false;
  }
  file->println(fileName);
  file->println();
  file->print("TIMESTAMP (MS)");
  file->print("\t");
  file->print("ID");
  file->print("\t");
  file->print("DATA");
  file->println();
  return true;
}


/** Reads all contents of a file
 *  @param *fileName Name of file on SD card
 *  @param *file File to be read
 *  @return Whether or not the file could be opened
 */
bool readFile(char *fileName, SdFile *file)
{
  if (!file->open(fileName, O_READ)) 
  {
    Serial.println("Failed to open file for read");
    return false;
  }
  int data; // changed from uint8_t to int -> check into this
  while ((data = file->read()) >= 0) 
  {
    delay(1);
    Serial.write(data);
  }
  file->close();
  return true;
}


/** Deletes all files on the SD card
 *  @param sd SD card object
 *  @return Whether or not the rm was successful
 */
bool deleteAllFiles(SdFat *sd)
{
  if (!sd->vwd()->rmRfStar())
  {
    Serial.println("Failed to delete all files");
    return false;
  }
  Serial.println("Deleted all files");
  return true;
}


/** Prints a CAN message to a file
 *  @param *message CAN message to be printed to the file
 *  @param *file File to be printed to
 * 
 */
void file_print_message(can_message_t *message, SdFile *file)
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

