#include <Time.h>
#include <can.h>
#include <FlexCAN.h>
#include <kinetis_flexcan.h>

FlexCAN CANbus(500000);
static CAN_message_t rxMsg;
static unsigned int rxCount;

void setup(void)
{
    CANbus.begin();
    Serial.begin(115200);
}

void loop(void)
{
  int count = 0;
  while (CANbus.read(rxMsg)) 
  {
    rxCount++;
    Serial.print(millis(),DEC);
    Serial.print(" ");
    Serial.print(rxMsg.id, HEX);
    Serial.print(" ");
    for (count = 0; count < rxMsg.len; count++)
    {
      Serial.print(rxMsg.buf[count], HEX);
      Serial.print(" ");
    }
  Serial.print("\n");
  }
}

