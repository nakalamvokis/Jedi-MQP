/* EEPROM Read
 *
 * Reads the value of each byte of the EEPROM and prints it 
 * to the computer.
 */

#include <EEPROM.h>

const unsigned int EEPROM_DATA_SIZE = 2048;   // 2048 Bytes (2 KB) of EEPROM data

int address = 0;
byte value;

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  value = EEPROM.read(address);   // read a byte from the current address
  
  Serial.print(address);
  Serial.print("\t");
  Serial.print(value, DEC);
  Serial.println();
  
  address++; // advance to next EEPROM address
 
  if (address == EEPROM_DATA_SIZE)  // reached end of EEPROM
  {
    exit(0);
  }
}
