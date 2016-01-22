
/*
  * File Name: SDCard.h
  * Date: 1/21/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  *
  * 
*/

#ifndef SDCARD_H
#define SDCARD_H

/* INCLUDES */
#include <SdFat.h>
#include "CANMessage.h"

/* FUNCTION PROTOTYPES */
bool configure_file(char *fileName, SdFile *file);
bool readFile(char *fileName, SdFile *file);
bool deleteAllFiles(SdFat *sd);
void file_print_message(can_message_t *message, SdFile *file);

#endif // SDCARD_H

