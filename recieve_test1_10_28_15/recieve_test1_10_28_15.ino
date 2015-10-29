/*
 * File Name: recieve_test1_10_28_15
 * Date: 10/28/2015
 * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
 * 
 * Brief: This program logs CAN Bus packets monitored on a 2014 Nissan Altima. The 
 *        CAN Bus Shield is initialized to read at a data rate of 500 Kbps. It then reads 
 *        the OBD-II port and writes the ID and payload of each packet monitored to the serial port.
 *        
 * Notes: 
 *        - The "CAN_BUS_Shield-master" library must be added to your Arduino IDE for this
 *          program to compile successfully.
 *        - This program is written for the SEEED CAN Bus Shield.
 *        - This code has only been tested on a 2014 Nissan Altima. It may work on other
 *          vehicles, but this is not certain.
 */

#include <SPI.h>
#include <mcp_can.h>


// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
	Serial.begin(115200);
	
	int i = 15; // 500 kbps
	if(CAN_OK == CAN.begin(i))
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
	unsigned char len = 0;
	unsigned char buf[8];
	
	if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
	{
		CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
		
		unsigned long canId = CAN.getCanId();
		
		if ((canId == 0x2DE) || (canId == 0x2))
		{
			
			Serial.println("-----------------------------");
			Serial.print("get data from ID: ");
			Serial.println(canId, HEX);
			for(int i = 0; i<len; i++)    // print the data
			{
				Serial.print(buf[i]);
				Serial.print("\t");
			}
			Serial.println();
		}
	}
}

/*********************************************************************************************************
	END FILE
*********************************************************************************************************/
