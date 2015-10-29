/*
 * File Name: Log_Data_2014_Nissan_Altima
 * Date: 10/28/2015
 * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
 * 
 * Brief: This program logs CAN Bus packets monitored on a 2014 Nissan Altima. The 
 *        CAN Bus Shield is initialized to read at a data rate of 500 Kbps. It then reads 
 *        the OBD-II port and writes the ID and payload of each packet monitored to the serial port.
 *        The packet information is comma seperated in order to easily be transferred to a .csv
 *        file.
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

/* SHIELD SETUP */
MCP_CAN CAN(SPI_CS_PIN);  // Set ship select pin

void setup()
{
    Serial.begin(115200);
  
    if (CAN_OK == CAN.begin(CAN_500KBPS)) // Initialize CAN reading at 500 Kbps
    {
        Serial.print("CAN BUS Shield init ok!");
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
  
    if(CAN_MSGAVAIL == CAN.checkReceive())    // check if data is coming in
    {
        CAN.readMsgBuf(&len, buf);            // read data,  len: data length, buf: data buffer
        unsigned long timestamp = millis();   // get time since program started in milliseconds
        unsigned long canId = CAN.getCanId(); // get the ID of the CAN message

        // MESSAGE PRINT FORMAT:
        // TIMESTAMP, ID NUMBER, DATA BYTE[0], DATA BYTE [1], ... , DATA BYTE[n]

        Serial.print(timestamp);   // print timestamp
        Serial.print(",");
        Serial.print(canId, HEX);  // print CAN message ID in HEX
        Serial.print(",");
        for(int i = 0; i<len; i++) // print each data byte in HEX
        {
            Serial.print(buf[i], HEX);
            Serial.print(",");
        }
        Serial.println();
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
