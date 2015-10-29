/*
 * File Name: Scan_Data_Rate_All_Vehicles
 * Date: 10/29/2015
 * Authors: Nicholas Kalamvokis, Santiago Rojas, Brian St. Germain, Mark Bentson
 * 
 * Brief: This program scans any CAN Bus active vehicle's OBD-II port for the data rate with
 *        which the vehicle communicates. Data rates to be tested range from 5 Kbps to 1000 Kbps.
 *        If it is determined that no data rate works, the program will warn the user that the
 *        vehicle is not compatible with the SEEED CAN Bus Shield.
 *        
 * Notes: 
 *        - The "CAN_BUS_Shield-master" library must be added to your Arduino IDE for this
 *          program to compile successfully.
 *        - This program is written for the SEEED CAN Bus Shield.
 */

/* INCLUDES */
#include <mcp_can.h>
#include <SPI.h>


/* TYPE DEFINITIONS */
struct DataRate
{
  int dataRateNumber;
  char* dataRateString;
};


/* CONSTANTS */
const int SPI_CS_PIN = 9; // SPI chip select pin set to 9
const DataRate DataRateTable[17] = 
{
    { 0,            "NONE" },
    { CAN_5KBPS,    "5 Kbps"},
    { CAN_10KBPS,   "10 Kbps"},
    { CAN_20KBPS,   "20 Kbps"},
    { CAN_31K25BPS, "31 Kbps"},
    { CAN_33KBPS,   "33 Kbps"},
    { CAN_40KBPS,   "40 Kbps"},
    { CAN_50KBPS,   "50 Kbps"},
    { CAN_80KBPS,   "80 Kbps"},
    { CAN_83K3BPS,  "83 Kbps"},
    { CAN_95KBPS,   "95 Kbps"},
    { CAN_100KBPS,  "100 Kbps"},
    { CAN_125KBPS,  "125 Kbps"},
    { CAN_200KBPS,  "200 Kbps"},
    { CAN_250KBPS,  "250 Kbps"},
    { CAN_500KBPS,  "500 Kbps"},
    { CAN_1000KBPS, "1000 Kbps"}
};


/* SHIELD SETUP */
MCP_CAN CAN(SPI_CS_PIN);  // Set chip select pin


void setup()
{
    Serial.begin(115200);
}


void loop()
{
    //unsigned char len = 0;  // length of data
    //unsigned char buf[8];   // 8 byte data buffer

    int dataRate = 0;

    for (dataRate = CAN_5KBPS; dataRate <= CAN_1000KBPS; dataRate++) // loop through and check all data rates
    {
        bool messageFound = false;
        if (CAN_OK == CAN.begin(dataRate)) // Initialize CAN reading the data rate
        {
            int counter = 0;
            while (counter < 10) // 100 attempts to read from the OBD-II port, each with a 10 ms wait (1 second of polling for data)
            {
                if (CAN_MSGAVAIL == CAN.checkReceive()) // check if data was recieved
                {
                    messageFound = true;
                    break;
                }
                counter++;
                delay(10); // wait 10 ms
            }
        }
        if (messageFound) // a message was recieved, data rate found
        {
            Serial.print("Data rate detected: ");
            Serial.print(getDataRateString(dataRate));
            Serial.println();
            break;
        }
        else if (dataRate == CAN_1000KBPS) // reached last data rate without detecting traffic
        {
          Serial.println("No data rate found.");
          Serial.println("This vehicle is not SEEED CAN Shield compatable.");
        }
    }
    Serial.println("Exiting program in 30 seconds...");
    delay(30000); // wait 30 seconds before exiting
    exit(0);
}

char* getDataRateString(int dataRate)
{
    return DataRateTable[dataRate].dataRateString;
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
