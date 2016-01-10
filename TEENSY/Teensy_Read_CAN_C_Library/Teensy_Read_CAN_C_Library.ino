/*
  * File Name: Teensy_Read_CAN_C_Library
  * Date: 1/2/2016
  * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
  *
  * Brief:
  *
  * Notes:
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
  can_message_t *buffer_start; // start of data buffer
  can_message_t *buffer_end;   // end of data buffer 
  size_t capacity;    // maximum number of items in buffer
  size_t itemSize;    // size of each item in buffer
  can_message_t *head;         // pointer to head
  can_message_t *tail;         // pointer to tail
  bool hasWrapped;    // whether or not circular buffer has wrapped around
} circular_buffer_t;

/* FUNCTION PROTOTYPES */
bool can_fifo_read(FLEXCAN_frame_t *frame);
void serial_print_frame(FLEXCAN_frame_t frame);
void serial_print_can_message(can_message_t* message);
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item);
void cicular_buffer_pop(circular_buffer_t *cb, can_message_t *item);
void circular_buffer_dump(circular_buffer_t *cb);
void test_circular_buffer(circular_buffer_t *cb);

/* CONSTANTS */
#define CIRCULAR_BUFFER_CAPACITY 250

/* GLOBALS */

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
  Serial.println("DIAG: Init buffer");
  cb->buffer_start = (can_message_t *) malloc(capacity * itemSize);
  cb->buffer_end = (cb->buffer_start + (capacity * itemSize));// check this, not sure how to make buffer_end
  cb->capacity = capacity;
  cb->itemSize = itemSize;
  cb->head = cb->buffer_start;
  cb->tail = cb->buffer_start;
  cb->hasWrapped = false;
}

/** Frees circular buffer
 *  @param *cb Circular buffer struct to be freed
 */
void circular_buffer_free(circular_buffer_t *cb)
{
  free(cb->buffer_start);
  free(cb->buffer_end);
  /*free(cb->capacity);
  free(cb->itemSize);
  free(cb->head);
  free(cb->tail);*/
}

/** Initializes circular buffer
 *  @param *cb Circular buffer struct to be initialized
 *  @param *item Data to be pushed to the circular buffer
 */
void circular_buffer_push(circular_buffer_t *cb, can_message_t *item)
{
  memcpy(cb->head, item, cb->itemSize);
  cb->head++;
  if (cb->head == cb->buffer_end)
  {
    cb->head = cb->buffer_start;
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
  if (cb->tail == cb->buffer_end)
  {
    cb->tail = cb->buffer_start;
  }
}

/** Dumps all circular buffer data to the serial port - for debug
 *  @param *cb Circular buffer struct to be dumped to serial
 */
void circular_buffer_dump(circular_buffer_t *cb)
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
    readStart = cb->buffer_start;
  }
  
  readEnd = cb->head;
  currentMessage = readStart;
  
  // iterate through circular buffer while no messages have been read and we haven't reached the end of the data
  while ((messageCount == 0) || (currentMessage != readEnd))
  {
    serial_print_can_message(currentMessage);
    currentMessage++;
    messageCount++;
  }
  Serial.print("\nDIAG: Message Count - ");
  Serial.print(messageCount, DEC);
  Serial.print(", expected - ");
  Serial.print(cb->capacity, DEC);
}

void test_circular_buffer(circular_buffer_t *cb)
{
  uint16_t numItems = 0;
  can_message_t newMessage;
  for (numItems = 0; numItems < cb->capacity; numItems++)
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

  Serial.println("DIAG: Dumping Circular Buffer");
  circular_buffer_dump(cb);
}


void setup(void)
{
  Serial.begin(115200);
  FLEXCAN_config_t can_config;
  
  can_config.presdiv   = 1;  // Prescale division factor.
  can_config.propseg   = 2;  // Prop Seg length.
  can_config.rjw       = 1;  // Sychronization Jump Width
  can_config.pseg_1    = 7;  // Phase 1 length
  can_config.pseg_2    = 3;  // Phase 2 length

  FLEXCAN_init(can_config);


}

void loop(void)
{
  circular_buffer_t cb;
  Serial.println("DIAG: Init Circular Buffer");
  circular_buffer_init(&cb, CIRCULAR_BUFFER_CAPACITY, sizeof(can_message_t));

  Serial.println("DIAG: Adding Data to Circular Buffer");
  test_circular_buffer(&cb);

/*
  FLEXCAN_frame_t newFrame;
  while (true) 
  {
    if (can_fifo_read(&newFrame))
    {
      serial_print_frame(newFrame);
    }
  }*/
 // circular_buffer_free(&cb);
  Serial.println();
  delay(10000);
}

