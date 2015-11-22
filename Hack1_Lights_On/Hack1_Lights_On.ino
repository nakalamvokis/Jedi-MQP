#include <mcp_can.h>
#include <SPI.h>

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
unsigned char LIGHTS_PID = 0x60D;       // Process ID for lights packets
unsigned char LIGHTS_ON_PACKET[8] = {0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00}; // Lights on packet data
const int LIGHTS_PACKET_SIZE = 8;   // lights on packet payload size
const int LIGHTS_ON_TRANSMIT_DELAY = 100; // delay time between lights packet transfer

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
    Serial.begin(115200);
    if(CAN_OK == CAN.begin(CAN_500KBPS))                   // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init ok!");
    }
}

void loop()
{
    CAN.sendMsgBuf(LIGHTS_PID, 0, LIGHTS_PACKET_SIZE, LIGHTS_ON_PACKET);  // send lights on packet
    delay(LIGHTS_ON_TRANSMIT_DELAY);                                      // delay 100ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
