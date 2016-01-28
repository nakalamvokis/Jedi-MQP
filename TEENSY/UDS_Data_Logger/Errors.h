/*
  * @file Errors.h
  * @author Nicholas Kalamvokis
  * @date 1/25/2016
  *
  *
*/

#ifndef ERRORS_H
#define ERRORS_H

/* INCLUDES */
#include <can.h>

/* DEFINES */
#define ERROR_CONTEXT_SIZE 100

/* ENUMS */
enum Error_e
{
  eERR_NONE = 0,
  eERR_SD_FAILED_INIT,
  eERR_SD_FAILED_FILE_OPEN_FOR_READ,
  eERR_SD_FAILED_FILE_OPEN_FOR_WRITE,
  eERR_SD_FAILED_FILE_DELETE,
  eERR_SD_FAILED_TO_CREATE_DIRECTORY,
  eERR_SD_LOST_COMMUNICATIONS,
  eERR_UNABLE_TO_SYNC_RTC
};

enum ErrorType_e
{
  eERRTYPE_NONE = 0,
  eERRTYPE_AUTO_RESUME,
  eERRTYPE_NON_RECOVERABLE
};

/* STRUCTS */
typedef struct {
  Error_e error;
  ErrorType_e errorType;
  char errorMessage[ERROR_CONTEXT_SIZE];
} error_t;

/* FUNCTION PROTOTYPES */
ErrorType_e GetErrorType(Error_e error);
char *GetErrorMessage(Error_e error);
void HandleError(Error_e error);
void Shutdown();

#endif // ERRORS_H

