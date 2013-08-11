/*
  EEPROM Manager Library
  ----------------------
  By Jac Goudsmit
  Distributed under the BSD (3-clause) license.
  http://github.com/jacgoudsmit/EEPROM_mgr
*/

#include <Arduino.h>

#include "EEPROM_mgr.h"

// Static data for the EEPROM_mgr library
word EEPROM_mgr::nextaddr;
EEPROM_mgr *EEPROM_mgr::list;
