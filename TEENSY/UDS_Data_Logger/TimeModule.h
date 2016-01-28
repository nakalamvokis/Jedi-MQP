/*
  * File Name: TimeModule.h
  * Date: 1/27/2016
  * Author: Nicholas Kalamvokis
  *
  * 
*/

#ifndef TIMEMODULE_H
#define TIMEMODULE_H

/* INCLUDES */
#include <Time.h>  
#include <Wire.h>
#include <DS1307RTC.h>
#include <stdio.h>

/* FUNCTION PROTOTYPES */
bool RTCInit();
void SetTimestamp(char *timestamp, size_t strLen);
void PrintTime();

#endif // TIMEMODULE_H

