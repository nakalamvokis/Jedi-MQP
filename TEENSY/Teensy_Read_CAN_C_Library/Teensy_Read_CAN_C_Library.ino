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
  can_message_t *buffer;  // data buffer
  size_t capacity;        // maximum number of items in buffer
  size_t itemSize;        // size of each item in buffer
  void *head;             // pointer to head
  void *tail;             // pointer to tail
} circular_buffer_t;

/* FUNCTION PROTOTYPES */
bool can_fifo_read(FLEXCAN_frame_t *frame);
void serial_print_frame(FLEXCAN_frame_t frame);
void circular_buffer_init(circular_buffer_t *cb, size_t capacity, size_t itemSize);
void circular_buffer_free(circular_buffer_t *cb);
void circular_buffer_push(circular_buffer_t *cb, void *item);


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
void serial_print_frame(FLEXCAN_frame_t frame)
{
    int frameLength = frame.dlc;
    int dataIndex = 0;
    Serial.println(millis(), DEC);
    Serial.println(frame.id, HEX);
    for (dataIndex = 0; dataIndex < frameLength; dataIndex++)
    {
        Serial.println(frame.data[dataIndex]);
    }
}


void setup(void)
{
  Serial.begin(115200);
   
  FLEXCAN_config_t can_config;
  
  can_config.presdiv   = 1;  /*!< Prescale division factor. */
  can_config.propseg   = 2;  /*!< Prop Seg length. */
  can_config.rjw       = 1;  /*!< Sychronization Jump Width*/
  can_config.pseg_1    = 7;  /*!< Phase 1 length */
  can_config.pseg_2    = 3;  /*!< Phase 2 length */

  FLEXCAN_init(can_config);
}

void loop(void)
{
  FLEXCAN_frame_t newFrame;
  while (true) 
  {
    if (can_fifo_read(&newFrame))
    {
      serial_print_frame(newFrame);
    }
  }
}







