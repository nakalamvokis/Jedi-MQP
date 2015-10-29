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
    if(CAN_OK == CAN.begin(15))                   // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init ok!");
    }
}


unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 27, 169};
void loop()
{
    // send data:  id = 0x2DE, standrad frame, data len = 8, stmp: data buf
    CAN.sendMsgBuf(0x2DE, 0, 8, stmp);
    delay(500);                       // send data per 100ms
 //   stmp[7] = 168;
   // CAN.sendMsgBuf(734, 0, 8, stmp);
   // delay(10000);  
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
