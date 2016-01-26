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
  {eERR_NONE,                       eERRTYPE_NONE,              ""},
  {eERR_SD_FAILED_INIT,             eERRTYPE_NON_RECOVERABLE,   "SD card failed initialization"},
  {eERR_SD_FAILED_FILE_OPEN_READ,   eERRTYPE_AUTO_RESUME,       "SD card failed to open a file for read"},
  {eERR_SD_FAILED_FILE_OPEN_WRITE,  eERRTYPE_NON_RECOVERABLE,   "SD card failed to open a file for write"},
  {eERR_SD_FAILED_FILE_DELETE,      eERRTYPE_NON_RECOVERABLE,   "SD card failed to delete a file"}
};

/** Gets the error type for an error
 * @param error Error to be searched
 */
ErrorType_e GetErrorType(Error_e error)
{
  return ErrorTable[error].errorType;
}

/** Gets the error message for an error
 * @param error Error to be searched
 */
char *GetErrorMessage(Error_e error)
{
  return ErrorTable[error].errorMessage;
}

/** Handles a recorded error
 * @param error Error flagged
 */
void HandleError(Error_e error)
{
  Serial.println(GetErrorMessage(error));
  if (GetErrorType(error) == eERRTYPE_NON_RECOVERABLE)
  {
    Shutdown();
  }
}

/** Executes a shutdown
 */
void Shutdown()
{
  Serial.println("Shutting Down...");
  while(true);
}

