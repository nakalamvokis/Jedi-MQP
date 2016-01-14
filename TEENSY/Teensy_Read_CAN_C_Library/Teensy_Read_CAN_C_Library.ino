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
#include <Time.h>
#include <can.h>

/* STRUCTS */
typedef struct {
  uint32_t timestamp; // Timestamp - milliseconds since runtime
  uint8_t len;        // Data length code [0 - 8]
  uint16_t id;        // Arbitration ID
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

/* FUNCTION PROTOTYPES */
void can_config_init(FLEXCAN_config_t *canConfig);
bool can_fifo_read(FLEXCAN_frame_t *frame);
void serial_print_frame(FLEXCAN_frame_t frame);
void serial_print_can_message(can_message_t* message);
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item);
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item);
int circular_buffer_dump(circular_buffer_t *cb);
void test_circular_buffer(circular_buffer_t *cb, uint16_t itemCount);

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY 2000

/* GLOBAL VARIABLES */
circular_buffer_t cb;
uint32_t count;
bool attackOccured;


/* COMPILER SWITCHES */
//#define DIAG 1


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
    Serial.println(millis(), DEC);
    Serial.println(frame->id, HEX);
    for (dataIndex = 0; dataIndex < frameLength; dataIndex++)
    {
        Serial.println(frame->data[dataIndex]);
    }
}


/** Prints a message to the serial port
 *  @param message Message struct containing message data
 */
void serial_print_can_message(can_message_t* message)
{
  uint8_t currentData = 0;
  Serial.print(message->timestamp, DEC);
  Serial.print(" ");
  Serial.print(message->id, DEC);
  Serial.print(" ");
  for (currentData = 0; currentData < message->len; currentData++)
  {
    Serial.print(message->data[currentData]);
    Serial.print(" ");
  }
  Serial.println();
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

/** Dumps all circular buffer data to the serial port - for debug
 *  @param *cb Circular buffer struct to be dumped to serial
 *  @return messageCount Number of messages read from circular buffer
 */
int circular_buffer_dump(circular_buffer_t *cb)
{
  can_message_t *readStart = NULL;
  can_message_t *readEnd = NULL;
  can_message_t *currentMessage = NULL;
  uint16_t messageCount = 0;

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
  
  // iterate through circular buffer while no messages have been read and we haven't reached the end of the data
  while ((messageCount == 0) || (currentMessage != readEnd))
  {
    serial_print_can_message(currentMessage);
    currentMessage++;
    if(currentMessage == cb->bufferEnd)
    {
      currentMessage = cb->bufferStart;
    }
    messageCount++;
  }
  return messageCount;
}

void test_circular_buffer(circular_buffer_t *cb, uint16_t itemCount)
{
  uint16_t numItems = 0;
  uint16_t readResult = 0;
  can_message_t newMessage;
  for (numItems = 0; numItems < itemCount; numItems++)
  {
    newMessage.timestamp = millis();
    newMessage.id = numItems;
    newMessage.len = 8;
    int currData = 0;
    for (currData = 0; currData < (&newMessage)->len; currData++)
    {
      newMessage.data[currData] = 8;
    }
    
    circular_buffer_push(cb, &newMessage);
  }
  Serial.println("\nDIAG: Dumping Circular Buffer");
  readResult = circular_buffer_dump(cb);
  Serial.print("\nDIAG: Message Count - ");
  Serial.print(readResult, DEC);
  Serial.print(", expected - ");
  Serial.print(cb->capacity, DEC); 
}


void setup(void)
{
  Serial.begin(115200);
  // CAN network configuration
  FLEXCAN_config_t canConfig;
  can_config_init(&canConfig);
  FLEXCAN_init(canConfig);

  // Circular buffer configuration
  circular_buffer_init(&cb, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));

  count = 0;
  attackOccured = false;
}


void loop(void)
{
  #ifdef DIAG
    Serial.println("\nDIAG: Init Circular Buffer");
    Serial.println("\nDIAG: Adding Data to Circular Buffer");
    // test_circular_buffer(&cb, 100);
    Serial.println();
    // test_circular_buffer(&cb, 250);
    Serial.println();
    test_circular_buffer(&cb, 20030);
    Serial.println();
    circular_buffer_free(&cb);

  #else
    //Serial.println(count);
    if (attackOccured)
    {
      Serial.println(count, DEC);
      Serial.println("Attack Occured");
      delay(10000);
    }
    else //(can_fifo_read(&newFrame)))
    {
      FLEXCAN_frame_t newFrame;

      // TEST
      newFrame.dlc = 8;
      newFrame.id = count;
      newFrame.data[0] = 1;
      newFrame.data[1] = 2;
      newFrame.data[2] = 3;
      newFrame.data[3] = 4;
      newFrame.data[4] = 5;
      newFrame.data[5] = 6;
      newFrame.data[6] = 7;
      newFrame.data[7] = 8;
      // END TEST

      if (count > 1000)
      {
        circular_buffer_dump(&cb);
        //circular_buffer_free(&cb);
        //attackOccured = true;
        //Serial.println("Attack found");
      }
      else
      {
        Serial.println("No attack");
        can_message_t newMessage;
        newMessage.timestamp = millis();
        memcpy(&(newMessage.len), &(newFrame.dlc), sizeof(newFrame.dlc));
        memcpy(&(newMessage.id), &(newFrame.id), sizeof(newFrame.id));
        memcpy(&(newMessage.data), &(newFrame.data), sizeof(newFrame.data));   
        circular_buffer_push(&cb, &newMessage);
        count++;
      }
    }
  #endif
}

