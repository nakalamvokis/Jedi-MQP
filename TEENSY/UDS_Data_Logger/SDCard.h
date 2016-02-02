/*
  * @file SDCard.h
  * @author Nicholas Kalamvokis
  * @date 1/21/2016
  *
  * 
*/

#ifndef SDCARD_H
#define SDCARD_H

/* INCLUDES */
#include <SdFat.h>
#include <Time.h>
#include <DS1307RTC.h>
#include "CANMessage.h"
#include "Errors.h"

/* FUNCTION PROTOTYPES */
bool SdInit(SdFat *sd, uint8_t chipSelect);
void dateTime(uint16_t *date, uint16_t *time);
bool MakeDirectory(char *dirName, SdFat *sd);
bool ConfigureDataFile(char *filePath, char* fileName, SdFile *file);
void OpenNewDataFile(SdFile *file, char *directory, const char *fileName, uint32_t fileNumber, size_t nameSize, size_t pathSize);
bool ReadFile(char *fileName, SdFile *file);
bool DeleteAllFiles(SdFat *sd);
void FileWriteMessage(can_message_t *message, SdFile *file);
bool CheckStatus(SdFat *sd);
bool SetFileCreateTime(SdFile *file);
bool SetFileEditTime(SdFile *file);
bool SetFileAccessTime(SdFile *file);

#endif // SDCARD_H

