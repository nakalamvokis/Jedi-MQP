/*
  * File Name: SDCard.h
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#ifndef SDCARD_H
#define SDCARD_H

/* INCLUDES */
#include <SdFat.h>
#include "CANMessage.h"
#include "Errors.h"

/* FUNCTION PROTOTYPES */
bool SdInit(SdFat *sd, uint8_t chipSelect);
bool MakeDirectory(char *dirName, SdFat *sd);
bool ConfigureFile(char *fileName, SdFile *file);
bool ReadFile(char *fileName, SdFile *file);
bool DeleteAllFiles(SdFat *sd);
void FilePrintMessage(can_message_t *message, SdFile *file);
void FilePrintString(char *text, SdFile *file);
bool CheckStatus(SdFat *sd);

#endif // SDCARD_H

