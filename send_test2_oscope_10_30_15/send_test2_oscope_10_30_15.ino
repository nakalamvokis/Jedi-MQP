#include <mcp_can.h>
#include <SPI.h>

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
unsigned char LIGHTS_PID = 0x60D;
unsigned char LIGHTS_ON_PACKET[8] = {0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00};
const int LIGHTS_PACKET_SIZE = 8;
const int LIGHTS_ON_TRANSMIT_DELAY = 100;

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
    // send data:  id = 0x2DE, standard frame, data len = 8, data: data buf
    CAN.sendMsgBuf(LIGHTS_PID, 0, LIGHTS_PACKET_SIZE, LIGHTS_ON_PACKET);
    delay(LIGHTS_ON_TRANSMIT_DELAY);                       // send data every 50ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
