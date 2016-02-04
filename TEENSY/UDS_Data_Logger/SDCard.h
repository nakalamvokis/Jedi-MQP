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
void ConfigureDataFile(SdFile *file, char* fileName);
bool OpenNewDataFile(SdFile *file, char *filePath, char *fileName);
bool OpenDataFile(SdFile *file, char *filePath);
bool ReadFile(char *fileName, SdFile *file);
bool DeleteAllFiles(SdFat *sd);
void FileWriteMessage(can_message_t *message, SdFile *file);
bool CheckStatus(SdFat *sd);
bool SetFileCreateTime(SdFile *file);
bool SetFileEditTime(SdFile *file);
bool SetFileAccessTime(SdFile *file);

#endif // SDCARD_H

