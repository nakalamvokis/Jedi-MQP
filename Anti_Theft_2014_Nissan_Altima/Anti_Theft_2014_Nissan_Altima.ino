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
  *        - Packet Composition:  {Byte[0], Byte[1], Byte[2], Byte[3], Byte[4], ... , Byte[n]} 
  *        - Byte Composition:    {Bit[7], Bit[6], Bit[5], Bit[4], Bit[3], Bit[2], Bit[1], Bit[0]}
*/

/* INCLUDES */
#include <mcp_can.h>
#include <SPI.h>

/* CONSTANTS */
const unsigned int MAX_PAYLOAD_SIZE = 0x08;               // Maximum data size of a packet's payload
const unsigned int STANDARD_FRAME = 0;                    // Standard frame parameter

/* LOCKS, DOORS, AND LIGHTS STATE */
const unsigned int LIGHTS_AND_DOORS_PID = 0x60D;          // Process ID cooresponding to lights and doors
/* Global to store previously read lights and doors packet data - initailized to all 0xFF in order to detect whether or not it has been set */
unsigned char previousLightsAndDoorsPacket [MAX_PAYLOAD_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const unsigned int DOORS_BYTE = 0x00;                     // Byte cooresponding to doors state
const unsigned int DRIVER_DOOR_BIT = 0x03;                // Bit position cooresponding to driver door
const unsigned int PASSENGER_DOOR_BIT = 0x04;             // Bit position cooresponding to passenger door
const unsigned int DRIVER_SIDE_BACK_DOOR_BIT = 0x05;      // Bit position cooresponding to driver side back door
const unsigned int PASSENGER_SIDE_BACK_DOOR_BIT = 0x06;   // Bit position cooresponding to passenger side back door

const unsigned int LOCKS_BYTE = 0x02;                     // Byte cooresponding to locks state
const unsigned int LOCKS_BIT = 0x04;                      // Bit position cooresponding to locks state (on/off)

const unsigned int LIGHTS_ON_DATA_BYTE = 0x00;            // Byte cooresponding to lights state
const unsigned int HIGH_BEAMS_DATA_BYTE = 0x01;           // Byte cooresponding to high beams state


/* TRUNK STATE */
const unsigned int TRUNK_PID = 0x358;                     // Process ID cooresponding to the trunk
/* Global to store previously read lights and doors packet data - initailized to all 0xFF in order to detect whether or not it has been set */
unsigned char previousTrunkPacket [MAX_PAYLOAD_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const unsigned int TRUNK_BYTE = 0x02;                     // Byte cooresponding to the trunk
const unsigned int TRUNK_BIT = 0x00;                      // Bit position cooresponding to the trunk


/* UDS DATA */
const unsigned char UDS_PID = 0x7E0;                      // Process ID for Universal Diagnostic System (UDS)


/* SHIELD SETUP */
const int SPI_CS_PIN = 9; // SPI chip select pin set to 9
MCP_CAN CAN(SPI_CS_PIN);  // Set ship select pin

void setup()
{
  Serial.begin(115200);

  if (CAN_OK == CAN.begin(CAN_500KBPS)) // Initialize CAN reading at 500 Kbps
  {
    //Serial.println("CAN BUS Shield init ok!");
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
  unsigned char newPacket[MAX_PAYLOAD_SIZE];   // 8 byte packet data buffer

  if (CAN_MSGAVAIL == CAN.checkReceive())   // check if data is coming in
  {
    CAN.readMsgBuf(&len, newPacket);  // read data,  len: data length, buf: data buffer
    unsigned long timestamp = millis();   // get time since program started in milliseconds
    unsigned long canId = CAN.getCanId(); // get the ID of the CAN message
    CheckPacket(canId, len, newPacket);
  }
}

void CheckPacket (int canId, unsigned char packetLength, unsigned char newPacket[])
{
  if (canId == UDS_PID) // Caught a UDS message
  {
    Serial.println("Malicious UDS message detected on the Network!");
    Serial.println("Please bring the vehicle to a stop, turn off the vehicle, and exit the vehicle as soon as possible");
  }

  else if (canId == LIGHTS_AND_DOORS_PID) // Check for changes in lights and doors packet
  {
    CheckLightsAndDoorsPacket(packetLength, newPacket, previousLightsAndDoorsPacket);
  }

  else if (canId = TRUNK_PID) // Check for changes in trunk packet
  {
    CheckTrunkPacket(packetLength, newPacket, previousTrunkPacket);
  }
}


void CheckLightsAndDoorsPacket (unsigned char packetLength, unsigned char *currentPacket, unsigned char *previousPacket)
{
  if (previousPacket[0] == 0xFF) // This is the first lights and doors packet we've monitored
  {
    memcpy(previousPacket, currentPacket, packetLength); // set the previously read packet to the most recently read packet
  }

  else // We have a packet to compare with, so check for changes
  {
    int currentByte = 0;
    for (currentByte = 0; currentByte < 3; currentByte++) // loop through all relevant bytes 0, 1, and 2
    {
      unsigned char changedBits = currentPacket[currentByte] ^ previousPacket[currentByte]; // determine which bits were changed since the last packet
      int currentBit = 0;
      for (currentBit = 0; currentBit < 8; currentBit++) // loop through all bits in the current byte
      {
        if ((changedBits >> currentBit) & 0x01) // check if current bit was changed, shift in order to check all bits
        {
          bool state = ((currentPacket[currentByte] >> currentBit) & 0x01); // checks what the current state of bit is
          switch (currentByte)
          {
            case DOORS_BYTE: // Check the byte that contains the state of the doors
              {
                switch (currentBit)
                {
                  case DRIVER_DOOR_BIT: // Driver door state has changed since the last monitored packet
                    {
                      if (state)
                      {
                        Serial.println("Driver side door opened");
                      }
                      else
                      {
                        Serial.println("Driver side door closed");
                      }
                      break;
                    }
                  case PASSENGER_DOOR_BIT: // Passenger door state has changed since the last monitored packet
                    {
                      if (state)
                      {
                        Serial.println("Passenger side door opened");
                      }
                      else
                      {
                        Serial.println("Passenger side door closed");
                      }
                      break;
                    }
                  case DRIVER_SIDE_BACK_DOOR_BIT: // Driver side back door state has changed since the last monitored packet
                    {
                      if (state)
                      {
                        Serial.println("Driver side back door opened");
                      }
                      else
                      {
                        Serial.println("Driver side back door closed");
                      }
                      break;
                    }
                  case PASSENGER_SIDE_BACK_DOOR_BIT: // Passenger side back door state has changed since the last monitored packet
                    {
                      if (state)
                      {
                        Serial.println("Passenger side back door opened");
                      }
                      else
                      {
                        Serial.println("Passenger side back door closed");
                      }
                      break;
                    }
                  default:
                    {
                      break;
                    }
                }
                break;
              }
            case LOCKS_BYTE: // Check the byte that contains the state of the door locks
              {
                switch (currentBit)
                {
                  case LOCKS_BIT: // Door locks state has changed since the last monitored packet
                    {
                      if (state)
                      {
                        Serial.println("Doors have been locked");
                      }
                      else
                      {
                        Serial.println("Doors have been unlocked");
                      }
                      break;
                    }
                  default:
                    {
                      break;
                    }
                }
                break;
              }
            default:
              {
                // do nothing
                break;
              }
          }
        }
      }
    }
    memcpy(previousPacket, currentPacket, MAX_PAYLOAD_SIZE); // set the previously read packet to the most recently read packet
  }
}

void CheckTrunkPacket (unsigned char packetLength, unsigned char *currentPacket, unsigned char *previousPacket)
{
  if (previousPacket[0] == 0xFF) // This is the first trunk packet we've monitored
  {
    memcpy(previousPacket, currentPacket, packetLength); // set the previously read packet to the most recently read packet
  }

  else
  {
    unsigned char changedBits = currentPacket[TRUNK_BYTE] ^ previousPacket[TRUNK_BYTE]; // determine which bits were changed since the last packet
    if ((changedBits >> TRUNK_BIT) & 0x01) // Check if the trunks state has changed
    {
      bool state = ((currentPacket[TRUNK_BYTE] >> TRUNK_BIT) & 0x01); // Check what the current state of bit is
      if (state) // Trunk was opened
      {
        Serial.println("Trunk was opened");
      }
      else // Trunk was closed
      {
        Serial.println("Trunk was closed");
      }
    }
    memcpy(previousPacket, currentPacket, packetLength); // set the previously read packet to the most recently read packet
  }
}


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
