// demo: CAN-BUS Shield, send data
#include <mcp_can.h>
#include <SPI.h>

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
    Serial.begin(115200);
    if(CAN_OK == CAN.begin(CAN_500KBPS))                   // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init ok!");
    }
}


unsigned char data[8] = {100, 100, 100, 100, 100, 100, 100, 100}; // send repeating data
void loop()
{
    // send data:  id = 0x2DE, standard frame, data len = 8, data: data buf
    CAN.sendMsgBuf(0x2DE, 0, 8, data);
    delay(50);                       // send data every 50ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
