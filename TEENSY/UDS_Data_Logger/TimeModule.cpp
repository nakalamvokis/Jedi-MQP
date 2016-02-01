/*
  * @file TimeModule.cpp
  * @author Nicholas Kalamvokis
  * @date 1/27/2016
  *
  * 
*/

/* INCLUDES */
#include "TimeModule.h"
#include "Errors.h"

/** Initializes RTC
 *  @return Status of the clock initialization
 */
bool RTCInit()
{
  bool status = true;
  setSyncProvider(RTC.get);
  if(timeStatus() != timeSet)
  {
    HandleError(eERR_UNABLE_TO_SYNC_RTC);
    status = false;
  }
  return status;
}

/** Creates a detailed timestamp in the format YYYY-MM-DD_HH-MM-SS
 *  This will be used to create the header timestamp on files saved the the SD Card
 *  @param *timestamp String to be populated with the timestamp
 *  @param strLen Maximum length of the timestamp string
 */
void SetTimestamp(char *timestamp, size_t strLen)
{
  time_t t = now();
  uint16_t Year = year(t);
  uint16_t Month = month(t);
  uint16_t Day = day(t);
  uint16_t Hour = hour(t);
  uint16_t Minute = minute(t);
  uint16_t Second = second(t);
  snprintf(timestamp, strLen, "%04d-%02d-%02d_%02d-%02d-%02d", Year, Month, Day, Hour, Minute, Second);
  delay(1000);
  Serial.println(timestamp);
  delay(1000);
}

/** Prints the current time to the serial port 
 */
void PrintTime()
{
  size_t len = 100;
  char timestamp[len];
  SetTimestamp(timestamp, len);
  Serial.println(timestamp);
}

