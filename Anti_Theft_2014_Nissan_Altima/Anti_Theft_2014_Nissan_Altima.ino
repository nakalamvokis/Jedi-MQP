/*
 * File Name: Log_Data_2014_Nissan_Altima
 * Date: 10/28/2015
 * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
 * 
 * Brief: This program serves as an Anti_Theft, Anti-Tampering system. It detects when UDS
 *        packets are being transferred by a malicious device on the CAN Network and signals
 *        to the user that the vehicle is at risk. There is also detection for doors and locks
 *        changing state as well as other peripherals changing state.
 *        
 * Notes: 
 *        - The "CAN_BUS_Shield-master" library must be added to your Arduino IDE for this
 *          program to compile successfully.
 *        - This program is written for the SEEED CAN Bus Shield.
 *        - This code has been written to protect a 2014 Nissan Altima.
 */


// MESSAGE PRINT FORMAT:
// TIMESTAMP, ID NUMBER, DATA BYTE[0], DATA BYTE [1], ... , DATA BYTE[n]

/* INCLUDES */
#include <mcp_can.h>
#include <SPI.h>

const unsigned char UDS_PID = 0x7E0;                      // Process ID for Universal Diagnostic System (UDS)
const unsigned char LIGHTS_AND_DOORS_PID = 0x60D;         // Process ID cooresponding to lights and doors
const unsigned char LIGHTS_ON_MASK = 0x06;                // Mask for lights on, cooresponds to ID 0x60D data byte[0]
const unsigned int LIGHTS_ON_DATA_BYTE = 0x00;            // Data byte for lights on in the 0x60D ID packets
const unsigned char HIGH_BEAMS_MASK = 0x08;     // Mask for high beams, cooresponds to ID 0x60D data byte[1]
const unsigned int HIGH_BEAMS_DATA_BYTE = 0x01; // Data byte for high beams on in the 0x60D ID packets
const unsigned int MAX_PAYLOAD_SIZE = 0x08;
unsigned char previousPacket [MAX_PAYLOAD_SIZE];
const unsigned int STANDARD_FRAME = 0; // Standard frame parameter

bool firstPacketRead = false; // flag for first packet read

/* CONSTANTS */
const int SPI_CS_PIN = 9; // SPI chip select pin set to 9

/* SHIELD SETUP */
MCP_CAN CAN(SPI_CS_PIN);  // Set ship select pin

void setup()
{
    Serial.begin(115200);
  
    if (CAN_OK == CAN.begin(CAN_500KBPS)) // Initialize CAN reading at 500 Kbps
    {
       // Serial.println("CAN BUS Shield init ok!");
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
    unsigned char currentPacket[MAX_PAYLOAD_SIZE];   // 8 byte data buffer
  
    if(CAN_MSGAVAIL == CAN.checkReceive())    // check if data is coming in
    {
        CAN.readMsgBuf(&len, currentPacket);            // read data,  len: data length, buf: data buffer
        unsigned long timestamp = millis();   // get time since program started in milliseconds
        unsigned long canId = CAN.getCanId(); // get the ID of the CAN message

        if (!firstPacketRead)
        {
            firstPacketRead = true;
        }

        else if (canId == UDS_PID)
        {
            Serial.println("Malicious UDS message detected on the Network!");
            Serial.println("Please bring the vehicle to a stop, turn off the vehicle, and exit the vehicle as soon as possible");
        }

        else if (canId == LIGHTS_AND_DOORS_PID)
        {
            if (((currentPacket[LIGHTS_ON_DATA_BYTE] ^ previousPacket[LIGHTS_ON_DATA_BYTE]) & LIGHTS_ON_MASK) == LIGHTS_ON_MASK) // check for a state change in the lights
            {
                if (currentPacket[LIGHTS_ON_DATA_BYTE] & LIGHTS_ON_MASK) // state was switched ON
                {
                    Serial.println("Lights have been switched on");
                }

                else // state was switched OFF
                {
                    Serial.println("Lights have been switched off");
                }
            }

            if (((currentPacket[HIGH_BEAMS_DATA_BYTE] ^ previousPacket[HIGH_BEAMS_DATA_BYTE]) & HIGH_BEAMS_MASK) == HIGH_BEAMS_MASK) // check for a state change in the high beams
            {
                if (currentPacket[HIGH_BEAMS_DATA_BYTE] & HIGH_BEAMS_MASK) // state was switched ON
                {
                    Serial.println("High beams have been switched on");
                }

                else // state was switched OFF
                {
                    Serial.println("High beams switched off");
                }
            }
        }

        memcpy(currentPacket, previousPacket, MAX_PAYLOAD_SIZE); // set the previously read packet to the most recently read packet
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
