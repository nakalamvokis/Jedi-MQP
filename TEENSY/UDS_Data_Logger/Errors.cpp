/*
  * File Name: Errors.cpp
  * Date: 1/25/2016
  * Author: Nicholas Kalamvokis
  *
  *
*/

#include "Errors.h"

error_t ErrorTable[]
{
  {eERR_NONE,                           eERRTYPE_NONE,              ""},
  {eERR_SD_FAILED_INIT,                 eERRTYPE_NON_RECOVERABLE,   "SD card failed initialization."},
  {eERR_SD_FAILED_FILE_OPEN_FOR_READ,   eERRTYPE_AUTO_RESUME,       "SD card failed to open a file for read."},
  {eERR_SD_FAILED_FILE_OPEN_FOR_WRITE,  eERRTYPE_NON_RECOVERABLE,   "SD card failed to open a file for write."},
  {eERR_SD_FAILED_FILE_DELETE,          eERRTYPE_NON_RECOVERABLE,   "SD card failed to delete a file."},
  {eERR_SD_FAILED_TO_CREATE_DIRECTORY,  eERRTYPE_NON_RECOVERABLE,   "SD card failed to create a directory."},
  {eERR_SD_LOST_COMMUNICATIONS,         eERRTYPE_NON_RECOVERABLE,   "Lost communications with SD card."}
};

/** Gets the error type for an error
 *  @param error Error to be searched
 */
ErrorType_e GetErrorType(Error_e error)
{
  return ErrorTable[error].errorType;
}

/** Gets the error message for an error
 *  @param error Error to be searched
 */
char *GetErrorMessage(Error_e error)
{
  return ErrorTable[error].errorMessage;
}

/** Handles a recorded error
 *  @param error Error flagged
 */
void HandleError(Error_e error)
{
  Serial.println(GetErrorMessage(error));
  switch (GetErrorType(error))
  {
    case eERRTYPE_NON_RECOVERABLE:
    {
      Shutdown();
      break;
    }
    case eERRTYPE_AUTO_RESUME:
    {
      // do nothing
      break;
    }
    default:
    {
      // do nothing
      break;
    }
  }
}

/** Executes a shutdown
 */
void Shutdown()
{
  Serial.println("Shutting Down...");
  while(true)
  {
    delay(1000);
  }
}

