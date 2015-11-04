/*
 * File Name: Log_Data_2014_Nissan_Altima
 * Date: 10/28/2015
 * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
 * 
 * Brief: This program periodically sends CAN Bus packets to indicate that the device should enter test mode.
 *        The CAN Bus Shield is initialized to read at a data rate of 500 Kbps. It then reads 
 *        the OBD-II port and writes the ID and payload of each packet monitored to the serial port.
 *        The packet information is comma seperated in order to easily be transferred to a .csv
 *        file. This code can be expanded to send test commands to the vehicle.
 *        
 * Notes: 
 *        - The "CAN_BUS_Shield-master" library must be added to your Arduino IDE for this
 *          program to compile successfully.
 *        - This program is written for the SEEED CAN Bus Shield.
 *        - This code has only been tested on a 2014 Nissan Altima. It may work on other
 *          vehicles, but this is not certain.
 */

/* INCLUDES */
#include <mcp_can.h>
#include <SPI.h>

/* CONSTANTS */
const int SPI_CS_PIN = 9; // SPI chip select pin set to 9

unsigned char DIAG_MODE_PAYLOAD[3]  = {0x02, 0x10, 0xC0}; // payload of tester present packet
const unsigned char DIAG_MODE_PID = 0x745;
const unsigned char DIAG_MODE_RESPONSE_PID = 0x765;
const unsigned int DIAG_MODE_PAYLOAD_SIZE = 3; // tester present packet payload size

unsigned char DIAG_BLINKER_OFF_PAYLOAD[5] = {0x04, 0x30, 0x2F, 0x20, 0x00};
unsigned char DIAG_RIGHT_BLINKER_PAYLOAD[5] = {0x04, 0x30, 0x2F, 0x20, 0x01};
unsigned char DIAG_LEFT_BLINKER_PAYLOAD[5] = {0x04, 0x30, 0x2F, 0x20, 0x02};
const unsigned int DIAG_BLINKER_PAYLOAD_SIZE = 5;
const unsigned int STANDARD_FRAME = 0; // Standard frame parameter


/* SHIELD SETUP */
MCP_CAN CAN(SPI_CS_PIN);  // Set ship select pin

void setup()
{
    Serial.begin(115200);
  
    if (CAN_OK == CAN.begin(CAN_500KBPS)) // Initialize CAN reading at 500 Kbps
    {
        Serial.println("CAN BUS Shield init ok!");
    }
  
    else
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
    }
}


void loop()
{
    unsigned char len = 0;  // length of data
    unsigned char buf[8];   // 8 byte data buffer
    unsigned long timestamp = millis();   // get time since program started in milliseconds
/*
    if (timestamp % DIAG_SEND_FREQ) // periodically send the tester present message
    {
      Serial.print(timestamp);
      Serial.print(": Sent tester present");
      Serial.println();
      CAN.sendMsgBuf(TESTER_PRESENT_PID, STANDARD_FRAME, TESTER_PRESENT_PAYLOAD_SIZE, TESTER_PRESENT_PAYLOAD);
    }
*/

    Serial.println("Sent diagnostic mode request");
    CAN.sendMsgBuf(DIAG_MODE_PID, STANDARD_FRAME, DIAG_MODE_PAYLOAD_SIZE, DIAG_MODE_PAYLOAD);
    delay(100);

    if(CAN_MSGAVAIL == CAN.checkReceive())    // check if data is coming in
    {
        timestamp = millis();
        CAN.readMsgBuf(&len, buf);            // read data,  len: data length, buf: data buffer
        
        unsigned long canId = CAN.getCanId(); // get the ID of the CAN message

        if (canId == DIAG_MODE_RESPONSE_PID) // check for valid diagnostic repsonse
        {
            Serial.print("Got diagnostic repsonse: ");

            // MESSAGE PRINT FORMAT:
            // TIMESTAMP, ID NUMBER, DATA BYTE[0], DATA BYTE [1], ... , DATA BYTE[n]
            
            Serial.print(canId, HEX);  // print CAN message ID in HEX
            Serial.print(" ");
            for(int i = 0; i<len; i++) // print each data byte in HEX
            {
                Serial.print(buf[i], HEX);
                Serial.print(" ");
            }
        }
        Serial.println();
    }
   
  int numBlinks = 0;
  while(numBlinks < 100)
  {
    int blinkCounter = 0;
    for (blinkCounter = 0; blinkCounter < 4; blinkCounter++)
    {
        CAN.sendMsgBuf(DIAG_MODE_PID, STANDARD_FRAME, DIAG_BLINKER_PAYLOAD_SIZE, DIAG_RIGHT_BLINKER_PAYLOAD);
        delay(100); // delay 100ms between blinker messages
    }

    for (blinkCounter = 0; blinkCounter < 4; blinkCounter++)
    {
        CAN.sendMsgBuf(DIAG_MODE_PID, STANDARD_FRAME, DIAG_BLINKER_PAYLOAD_SIZE, DIAG_BLINKER_OFF_PAYLOAD);
        delay(100); // delay 100ms between blinker messages
    }
    numBlinks++;
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
