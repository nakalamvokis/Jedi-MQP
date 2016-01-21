/*
  * File Name: Teensy_Read_CAN_C_Library
  * Date: 1/2/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  * Brief:
  *
  * Notes:
  * When developing this code, it was determined that the compiler is very picky, especially
  * with pointer arithmetic. For this reason, the circular buffer implements a counter rather
  * than pointer comparison for checking the buffer wrapping condition.
  * 
*/

/* INCLUDES */
#include <can.h>
#include <SPI.h>
#include <SdFat.h>
#include <Time.h>

/* STRUCTS */
typedef struct {
  uint32_t timestamp; // Timestamp - milliseconds since runtime
  uint8_t len;        // Data length code [0 - 8]
  uint32_t id;        // Arbitration ID
  uint8_t data[8];    // Data payload - maximum 8 bytes
} can_message_t;

typedef struct {
  can_message_t *bufferStart;   // start of data buffer
  can_message_t *bufferEnd;     // end of data buffer
  can_message_t *head;          // pointer to head
  can_message_t *tail;          // pointer to tail
  size_t capacity;              // maximum number of items in buffer
  size_t itemSize;              // size of each item in buffer
  bool hasWrapped;              // whether or not circular buffer has bufferEndped around
} circular_buffer_t;


typedef struct {
  can_message_t *bufferStart;   // start of data buffer
  can_message_t *bufferEnd;     // end of data buffer
  can_message_t *next;          // pointer to next free location
  size_t capacity;              // maximum number of items in buffer
  size_t itemSize;              // size of each item in buffer
} linear_buffer_t;


/* FUNCTION PROTOTYPES */
void create_file_timestamp(char *timestamp, size_t strLen);
bool configure_file(char *fileName, SdFile *file);
bool readFile(char *fileName, SdFile *file);
bool deleteAllFiles();
void can_config_init(FLEXCAN_config_t *canConfig);
bool can_fifo_read(FLEXCAN_frame_t *frame);
void serial_print_frame(FLEXCAN_frame_t frame);
int generate_frame(FLEXCAN_frame_t *frame);
void serial_print_can_message(can_message_t *message);
void file_print_message(can_message_t *message, SdFile *file);
void transpose_can_message(can_message_t *message, FLEXCAN_frame_t *frame);
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void circular_buffer_reinit(circular_buffer_t *cb);
void circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item);
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item);
int circular_buffer_dump(circular_buffer_t *cb);
void changeState(int newReadType, int newNetworkStatus);

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY      2000      // Maximum capacity of the circular buffer
#define LINEAR_BUFFER_CAPACITY        1000      // Maximum capacity of the linear buffer
#define UDS_ID                        0x7E8     // Arbitration ID of all UDS messages sent to the vehicle
#define TEST_PACKET_TRANSFER_DELAY    1         // Packet simulation send delay time
#define CIRCULAR_BUFFER               0         // Store messages into circular buffer
#define LINEAR_BUFFER                 1         // Store messages into linear buffer
#define NORMAL_TRAFFIC                0         // Normal CAN traffic
#define CORRUPT_TRAFFIC               1         // Corrupted CAN traffic
#define SD_CHIP_SELECT                10        // Chip select pin for SD card
#define TIMESTAMP_SIZE                40        // Size of timestamp string
#define MAX_CORRUPT_TRAFFIC_READINGS  50000      // Amount of corrupt CAN messages that will be recorded after each UDS message


/* GLOBAL VARIABLES */
circular_buffer_t cb;                 // Normal CAN traffic circular buffer
SdFat sd;                             // SD Card object
char fileTimestamp[TIMESTAMP_SIZE];   // Timestamp for each file saved to SD card, this marks the start time of the program
uint8_t readType;                     // Type of storage used to store messages
uint8_t networkStatus;                // Current status of CAN traffic (Normal or Corrupt) that decides how the data should be stored
uint32_t corruptMsgCount;             // Number of messages since UDS message was monitored
uint32_t fileNumber;                  // Number cooresponding to each UDS attack instance
uint32_t numUDSMessages;              // Number of UDS messages in a single attack
bool errorStatus;                     // System error status

// TEST //
bool UDS_ON = false;


/** Creates a detailed timestamp
 *  This will be used to create the header timestamp on files saved the the SD Card
 *  @param *timestamp String to be populated with the timestamp
 */
void create_file_timestamp(char *timestamp, size_t strLen)
{
  time_t t = now();
  uint16_t Year = year(t);
  uint16_t Month = month(t);
  uint16_t Day = day(t);
  uint16_t Hour = hour(t);
  uint16_t Minute = minute(t);
  uint16_t Second = second(t);
  sprintf(timestamp, "%02d/%02d/%04d  %02d:%02d:%02d", Month, Day, Year, Hour, Minute, Second);
}

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
  file->println(fileTimestamp);
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
  sd.remove(fileName);
  return true;
}


/** Deletes all files on the SD card
 *  @return Whether or not the rm was successful
 */
bool deleteAllFiles()
{
  if (!sd.vwd()->rmRfStar())
  {
    Serial.println("Failed to delete all files");
    return false;
  }
  Serial.println("Deleted all files");
  return true;
}


/** Initializes CAN configuration parameters
 *  @param canConfig CAN configuration struct to be set
 */
void can_config_init(FLEXCAN_config_t *canConfig)
{
  canConfig->presdiv   = 1;  // Prescale division factor.
  canConfig->propseg   = 2;  // Prop Seg length.
  canConfig->rjw       = 1;  // Sychronization Jump Width
  canConfig->pseg_1    = 7;  // Phase 1 length
  canConfig->pseg_2    = 3;  // Phase 2 length
}


/** Reads a CAN message from the FIFO queue
 *  @param *frame Pointer to frame struct to be populated with message data
 *  @return Whether or not a message was read from the FIFO queue
 */
bool can_fifo_read(FLEXCAN_frame_t *frame)
{
   if(FLEXCAN_fifo_avalible())
   {
      FLEXCAN_fifo_read(frame);
      return true;
   }
   return false;
}


/** Prints a frame to the serial port
 *  @param frame Frame struct containing message data
 */
void serial_print_frame(FLEXCAN_frame_t *frame)
{
  int frameLength = frame->dlc;
  int dataIndex = 0;
  Serial.print(millis(), DEC);
  Serial.print(" ");
  Serial.print(frame->id, HEX);
  Serial.print(" ");
  for (dataIndex = 0; dataIndex < frameLength; dataIndex++)
  {
    Serial.print(frame->data[dataIndex]);
    Serial.print(" ");
  }
  Serial.println();
}


/** Generates a random CAN frame
 *  This function will only be used for testing purposes
 * @param frame Frame to be filled with random data
 * @return If a frame was generated or not
 */
int generate_frame(FLEXCAN_frame_t *frame)
{
  uint8_t status = random(0, 1 + 1);
  if (status)
  {
    int dataCounter = 0;
    frame->dlc = random(0, 8 + 1);
    for (dataCounter = 0; dataCounter < frame->dlc; dataCounter++)
    {
      frame->data[dataCounter] = random(0, 255 + 1);
    }
    if (UDS_ON)
    {
      frame->id = random(0, 50000 + 1);
    }
    else
    {
      frame->id = random(0, 8192 + 1);
    }
    //serial_print_frame(frame);
  }
  return status;
}


/** Prints a message to the serial port
 *  @param message Message struct containing message data
 */
void serial_print_can_message(can_message_t *message)
{
  uint8_t currentData = 0;
  Serial.print(message->timestamp, DEC);
  Serial.print(" ");
  Serial.print(message->id, HEX);
  Serial.print(" ");
  for (currentData = 0; currentData < message->len; currentData++)
  {
    Serial.print(message->data[currentData], HEX);
    Serial.print(" ");
  }
  Serial.println();
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


/** Copies frame data to a CAN message
 *  This will be used to extract neccessary data from a frame in order to shorten the
 *  size of each packet in memory
 *  @param *message CAN message to be filled with data
 *  @param *frame Frame to be copied to CAN message
 */
void transpose_can_message(can_message_t *message, FLEXCAN_frame_t *frame)
{
  message->timestamp = millis();
  memcpy(&(message->len), &(frame->dlc), sizeof(frame->dlc));
  memcpy(&(message->id), &(frame->id), sizeof(frame->id));
  memcpy(&(message->data), &(frame->data), sizeof(frame->data));
}


/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param capacity Size of circular buffer
 *  @param itemSize Size of a single item in the circular buffer
 */
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize)
{
  cb->bufferStart = (can_message_t *) calloc(capacity, itemSize);
  cb->capacity = capacity;
  cb->itemSize = itemSize;
  cb->head = cb->bufferStart;
  cb->tail = cb->bufferStart;
  cb->bufferEnd = cb->bufferStart;
  cb->hasWrapped = false;
  cb->bufferEnd += capacity;
}


/** Reinitializes circular buffer
 *  @param *cb Circular buffer struct to be reinitialized
 */
void circular_buffer_reinit(circular_buffer_t *cb)
{
  cb->head = cb->bufferStart;
  cb->tail = cb->bufferStart;
  cb->hasWrapped = false;
}


/** Frees circular buffer
 *  @param *cb Circular buffer struct to be freed
 */
void circular_buffer_free(circular_buffer_t *cb)
{
  free(cb->bufferStart);
}


/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param *item Data to be pushed to the circular buffer
 */
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(cb->head, item, cb->itemSize);
  cb->head++;
  if (cb->head == cb->bufferEnd)
  {
    cb->head = cb->bufferStart;
    cb->hasWrapped = true;
  }
}


/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param *item Data to be populated with pop from circular buffer
 */
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(item, cb->tail, cb->itemSize);
  cb->tail++;
  if (cb->tail == cb->bufferEnd)
  {
    cb->tail = cb->bufferStart;
  }
}


/** Dumps all circular buffer data to a new file on the SD Card
 *  @param *cb Circular buffer struct to be dumped to SD
 *  @return messageCount Number of messages read from circular buffer
 */
int circular_buffer_dump(circular_buffer_t *cb)
{
  can_message_t *readStart = NULL;
  can_message_t *readEnd = NULL;
  can_message_t *currentMessage = NULL;
  uint16_t messageCount = 0;
  SdFile cbFile;

  char fileName[30];
  sprintf(fileName, "Before_UDS_Attack_%lu.txt", fileNumber);
  
  configure_file(fileName, &cbFile);

  if (cb->hasWrapped)
  {
    readStart = cb->head;
  }
  else
  {
    readStart = cb->bufferStart;
  }
  
  readEnd = cb->head;
  currentMessage = readStart;
  
  // iterate through circular buffer until we reach the end of the data
  while ((messageCount == 0) || (currentMessage != readEnd))
  {
    /*
    delay(5);
    Serial.print("CIRCULAR BUFFER MESSAGE #");
    Serial.print(count, DEC);
    Serial.print(" ");
    serial_print_can_message(currentMessage);
    */
   /* cbFile.print("MSG: ");
    cbFile.print(" ");
    cbFile.print(messageCount, DEC);
    cbFile.print(" ");*/
    file_print_message(currentMessage, &cbFile);
    currentMessage++;
    messageCount++;
    if(currentMessage == cb->bufferEnd)
    {
      currentMessage = cb->bufferStart;
    }
  }
  cbFile.close();
//  readFile(fileName, &cbFile);
  return messageCount;
}












/** Initializes linear buffer
 *  @param *lb linear buffer struct to be initialized
 *  @param capacity Size of linear buffer
 *  @param itemSize Size of a single item in the linear buffer
 */
void linear_buffer_init(linear_buffer_t *lb, size_t capacity, size_t itemSize)
{
  lb->bufferStart = (can_message_t *) calloc(capacity, itemSize);
  lb->capacity = capacity;
  lb->itemSize = itemSize;
  lb->next = cb->bufferStart;
  lb->bufferEnd = lb->bufferStart;
  lb->bufferEnd += capacity;
}


/** Reinitializes circular buffer
 *  @param *cb Circular buffer struct to be reinitialized
 */
void linear_buffer_reinit(linear_buffer_t *lb)
{
  lb-> = lb->bufferStart;
  lb->tail = lb->bufferStart;
}


/** Frees circular buffer
 *  @param *cb Circular buffer struct to be freed
 */
void circular_buffer_free(circular_buffer_t *cb)
{
  free(cb->bufferStart);
}


/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param *item Data to be pushed to the circular buffer
 */
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(cb->head, item, cb->itemSize);
  cb->head++;
  if (cb->head == cb->bufferEnd)
  {
    cb->head = cb->bufferStart;
    cb->hasWrapped = true;
  }
}


/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param *item Data to be populated with pop from circular buffer
 */
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(item, cb->tail, cb->itemSize);
  cb->tail++;
  if (cb->tail == cb->bufferEnd)
  {
    cb->tail = cb->bufferStart;
  }
}


/** Dumps all circular buffer data to a new file on the SD Card
 *  @param *cb Circular buffer struct to be dumped to SD
 *  @return messageCount Number of messages read from circular buffer
 */
int circular_buffer_dump(circular_buffer_t *cb)
{
  can_message_t *readStart = NULL;
  can_message_t *readEnd = NULL;
  can_message_t *currentMessage = NULL;
  uint16_t messageCount = 0;
  SdFile cbFile;

  char fileName[30];
  sprintf(fileName, "Before_UDS_Attack_%lu.txt", fileNumber);
  
  configure_file(fileName, &cbFile);

  if (cb->hasWrapped)
  {
    readStart = cb->head;
  }
  else
  {
    readStart = cb->bufferStart;
  }
  
  readEnd = cb->head;
  currentMessage = readStart;
  
  // iterate through circular buffer until we reach the end of the data
  while ((messageCount == 0) || (currentMessage != readEnd))
  {
    /*
    delay(5);
    Serial.print("CIRCULAR BUFFER MESSAGE #");
    Serial.print(count, DEC);
    Serial.print(" ");
    serial_print_can_message(currentMessage);
    */
   /* cbFile.print("MSG: ");
    cbFile.print(" ");
    cbFile.print(messageCount, DEC);
    cbFile.print(" ");*/
    file_print_message(currentMessage, &cbFile);
    currentMessage++;
    messageCount++;
    if(currentMessage == cb->bufferEnd)
    {
      currentMessage = cb->bufferStart;
    }
  }
  cbFile.close();
//  readFile(fileName, &cbFile);
  return messageCount;
}

























/** Sets parameters during a state change
 *  @param newReadType New 
 */
void changeState(int newReadType, int newNetworkStatus)
{
  readType = newReadType;
  networkStatus = newNetworkStatus;
  switch (newReadType)
  {
    case LINEAR_BUFFER:
    {
      Serial.println("Changing State: Circular -> Linear");
      numUDSMessages = 0;
      corruptMsgCount = 0;
      UDS_ON = true; // TEST SIMULATION - REMOVE LATER //
      break;
    }
    case CIRCULAR_BUFFER:
    {
      Serial.println("Changing State: Linear -> Circular");
      circular_buffer_reinit(&cb);
      fileNumber++;
      UDS_ON = false; // TEST SIMULATION - REMOVE LATER //
      break;
    }
    default:
    {
      break;
    }
  }
}

void setup(void)
{
  Serial.begin(115200);

  /* CAN Network Configuration */
  FLEXCAN_config_t canConfig;
  can_config_init(&canConfig);
  FLEXCAN_init(canConfig);

  /* Circular Buffer Configuration */
  circular_buffer_init(&cb, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));

  /* File Writing Configuration */
  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) 
  {
    Serial.println("Failed to init SD Card");
  }
  deleteAllFiles();
  create_file_timestamp(fileTimestamp, TIMESTAMP_SIZE);
  readType = CIRCULAR_BUFFER;
  networkStatus = NORMAL_TRAFFIC;

  /* Counters */
  corruptMsgCount = 0;
  fileNumber = 1;
  numUDSMessages = 0;
}


void loop(void)
{
    FLEXCAN_frame_t newFrame;

    switch (readType)
    { 
      case CIRCULAR_BUFFER:
      {
        if (generate_frame(&newFrame)) // (can_fifo_read(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          transpose_can_message(&newMessage, &newFrame);

          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            circular_buffer_dump(&cb);
            Serial.print("  Attack found: ");
            serial_print_can_message(&newMessage);
            delay(100);
            changeState(LINEAR_BUFFER, CORRUPT_TRAFFIC);
          }
          else
          {
            circular_buffer_push(&cb, &newMessage);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }

      case LINEAR_BUFFER:
      {
        if (generate_frame(&newFrame)) // (can_fifo_read(&newFrame)) // CURRENTLY SIMULATED
        {
          can_message_t newMessage;
          transpose_can_message(&newMessage, &newFrame);
          corruptMsgCount++;
          if (newMessage.id == UDS_ID) // UDS message detected, an attack occured
          {
            Serial.print("  Another UDS Message found: ");
            Serial.print(newMessage.id, HEX);
            Serial.println(" (Extending read time)");
            corruptMsgCount = 0;
            numUDSMessages++;
          }
          else if (corruptMsgCount >= MAX_CORRUPT_TRAFFIC_READINGS)
          {
            // write number of UDS messages to file here
            Serial.println("  Done recording UDS traffic");
            changeState(CIRCULAR_BUFFER, CORRUPT_TRAFFIC);
          }
          delay(TEST_PACKET_TRANSFER_DELAY); // TEST CODE - SIMULATES TIME BETWEEN MSG TRANSFERS
        }
        break;
      }
      default:
      {
        break;
      }
    }
}

