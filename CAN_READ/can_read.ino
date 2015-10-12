#include <SPI.h>
#include <SD.h>
#include <Canbus.h>
#include <Time.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>


File testFile;

void setup() {
  Serial.begin(9600); // sets data rate to 9600 baud (between arduino and PC)
  
  time_t t = now();
  char testFileName[30] = "CAN_test_file.txt";
  
  SD.begin(8); // initialize the sd card on the shield
  
  testFile = SD.open(testFileName, FILE_WRITE);
  
  
  // Initialize MCP2515 CAN controller at the specified speed
  // Here, we are testing for the CAN speed
  if(Canbus.init(CANSPEED_500))
  {
    Serial.println("CAN Init - 500 kbps worked!");
  }
  
  else if(Canbus.init(CANSPEED_250))
  {
    Serial.println("CAN Init - 250 kbps worked!");
  }
  
  else if(Canbus.init(CANSPEED_125))
  {
    Serial.println("CAN Init - 125 kbps worked!");
  }
  
  delay(1000);
  
}

void loop()
{ 
  tCAN message;
  int messageCount = 0;
  
  while (messageCount < 1000) // save file and exit after recieving 1000 messages
  {
    if (mcp2515_check_message()) // check if there is a new message waiting
    {
      if (mcp2515_get_message(&message)) // get the message and print contents to a file
      {
        messageCount++;
        
        testFile.println("Message number: " + messageCount);
        testFile.println("  ID: " + message.id);
        Serial.print("ID: " );
        Serial.println(message.id,HEX);
        
        testFile.print("  Data: ");
        Serial.print("Data: ");
        for(int i=0; i<message.header.length; i++)
        {
          testFile.print(message.data[i]);
          Serial.print(message.data[i],HEX);
          Serial.print(" ");
        }
        testFile.println("");
        Serial.println("");
      }
    }
  }
  testFile.close(); // close file
  exit(0); // exit program
}
