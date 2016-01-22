/*
  * File Name: CANMessage.h
  * Date: 1/21/2016
  * Author: Nicholas Kalamvokis
  *
  *
*/

#ifndef CANMESSAGE_H
#define CANMESSAGE_H

/* INCLUDES */
#include <can.h>
#include <string.h>

/* STRUCTS */
typedef struct {
  uint32_t timestamp; // Timestamp - milliseconds since runtime
  uint8_t len;        // Data length code [0 - 8]
  uint32_t id;        // Arbitration ID
  uint8_t data[8];    // Data payload - maximum 8 bytes
} can_message_t;

/* FUNCTION PROTOTYPES */
void can_config_init(FLEXCAN_config_t *canConfig);
bool can_fifo_read(FLEXCAN_frame_t *frame);
void serial_print_can_message(can_message_t *message);
void serial_print_frame(FLEXCAN_frame_t frame);
int generate_frame(FLEXCAN_frame_t *frame, uint16_t minID, uint16_t maxID);
void transpose_can_message(can_message_t *message, FLEXCAN_frame_t *frame);

#endif // CANMESSAGE_H

