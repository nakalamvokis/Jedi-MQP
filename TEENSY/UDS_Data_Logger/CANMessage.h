/*
  * @file CANMessage.h
  * @author Nicholas Kalamvokis
  * @date 1/21/2016
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
void CanConfigInit(FLEXCAN_config_t *canConfig);
bool CanFifoRead(FLEXCAN_frame_t *frame);
void SerialPrintCanMessage(can_message_t *message);
void SerialPrintFrame(FLEXCAN_frame_t frame);
int GenerateFrame(FLEXCAN_frame_t *frame, uint16_t minID, uint16_t maxID);
void TransposeCanMessage(can_message_t *message, FLEXCAN_frame_t *frame);

#endif // CANMESSAGE_H

