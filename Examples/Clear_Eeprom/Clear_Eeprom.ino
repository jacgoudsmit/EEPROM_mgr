/*
  Clear the entire EEPROM
  -----------------------
  Example using the EEPROM Manager library to erase the entire EEPROM
  By Jac Goudsmit
  Distributed under the BSD (3-clause) license.
  http://github.com/jacgoudsmit/EEPROM_mgr

*/

#include <arduino.h>
#include <stdlib.h>
#include <EEPROM_mgr.h>

// The highest usable EEPROM address is #defined in the Arduino header files
// as E2END. The value depends on the microcontroller that's used. For
// example, the AVR328 has 1KB of EEPROM, the AVR168 has 512 bytes.
// The following line declares a type that takes up the entire size of the
// EEPROM.
typedef char entire_eeprom[E2END + 1];

// Create an item that uses the entire EEPROM
EEPROM_item<entire_eeprom> everything;

void setup()
{
  // Fill the item with 0xFF to clear the EEPROM
  // We can't use the assignment operator here so we use the Data operator
  // to get a pointer to the internal data of the item, fill the data with
  // bytes containing value 255, and then store the item to the EEPROM.
  memset(everything.Data(), 255, sizeof(entire_eeprom));
  everything.Store();
}

void loop()
{
}
