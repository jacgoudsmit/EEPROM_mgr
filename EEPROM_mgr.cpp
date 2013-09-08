/*
  EEPROM Manager Library
  ----------------------
  By Jac Goudsmit
  Distributed under the BSD (3-clause) license.
  http://github.com/jacgoudsmit/EEPROM_mgr
*/

#include <Arduino.h>

#include "EEPROM_mgr.h"


//--------------------------------------------------------------------------
// Implementation for the "missing" EEPROM function
bool                                    // Returns true if all data matches
eeprom_verify_block(
  const void *ram_data,                 // Pointer to RAM data
  void *eeprom_data,                    // Pointer to EEPROM data
  size_t size)                          // Number of bytes to compare
{
  bool result = true;
  const byte *p = (const byte *)ram_data;
  const byte *e = (const byte *)eeprom_data;
      
  for (size_t n = 0; n < size; n++, e++)
  {
    if (eeprom_read_byte(e) != *p)
    {
      result = false;
      break;
    }
  }

  return result;
}


////////////////////////////////////////////////////////////////////////////
// Base class for EEPROM items
////////////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------------
// Static data for the EEPROM_mgr class
word                EEPROM_mgr::nextaddr;
EEPROM_mgr         *EEPROM_mgr::list;
word                EEPROM_mgr::signature;


//--------------------------------------------------------------------------
// Constructor
EEPROM_mgr::EEPROM_mgr(
  size_t size)                          // Size for data required by item
: m_addr(nextaddr)
, m_size(size)
{
  if ((!signature) && (size))
  {
    // Add item to linked list
    nextaddr += size;
    
    m_next = list;
    list = this;
  }
  else
  {
    // Set size to zero indicating that item could not be added to list
    m_size = 0;
  }
}
  

//--------------------------------------------------------------------------
// Destructor
EEPROM_mgr::~EEPROM_mgr()
{
  // Normally this should never be called but this code will clean
  // everything up nicely.
  
  // If this instance is not in the list because either the list was
  // already finalized when the item was created, or because the requested
  // size was zero, skip the code that removes the item from the list.
  if (m_size)
  {
    // If we're at the top of the list, removing is trivial
    // Otherwise, we need to find our predecessor.
    if (list == this)
    {
      list = m_next;
    }
    else
    {
      for (EEPROM_mgr *cur = list; cur->m_next; cur = cur->m_next)
      {
        if (cur->m_next == this)
        {
          cur->m_next = m_next;
          break;
        }
      }
    }

    // Invalidate the signature to let the rest of the code know that the
    // list is not in its normal state anymore
    signature = 0;
  }
}

  
//--------------------------------------------------------------------------
// Static function to save all values and write the signature
void
EEPROM_mgr::StoreAll()
{
  // Don't write to EEPROM if there are no items or if list not finalized
  if ((nextaddr) && (signature))
  {
    for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
    {
      cur->_Store();
    }

    // Don't write the signature if it's already there, to reduce wear
    if (!VerifySignature())      
    {
      eeprom_write_block(&signature, (void *)nextaddr, sizeof(signature));
    }
  }
}
  
  
//--------------------------------------------------------------------------
// Retrieve all values but only if the signature is correct
bool                                    // Returns true if values retrieved
EEPROM_mgr::RetrieveAll()
{
  bool result = false;
  
  // Don't read if there are no items or if list is not finalized
  if ((nextaddr) && (signature))
  {
    // If the signature doesn't match the EEPROM, don't trash the data
    result = VerifySignature();
    
    if (result)
    {
      for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
      {
        cur->_Retrieve();
      }
    }
  }
  
  return result;
}

  
//--------------------------------------------------------------------------
// Verify that all values in the EEPROM are equal to the stored values
bool                                    // Returns true if all values match
EEPROM_mgr::VerifyAll()
{
  // If the signature doesn't match, all bets are off.
  bool result = VerifySignature();
  
  if (result)
  {
    for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
    {
      if (!cur->_Verify())
      {
        result = false;
        break;
      }
    }
  }
  
  return result;
}
  
  
//--------------------------------------------------------------------------
// This should be called at the beginning of your sketch
bool 
EEPROM_mgr::Begin(
  bool storeifinvalid,
  bool storealways,
  bool wipeunusedareas,
  bool retrieveifvalid)
{
  bool result = false;
  
  // Calculate the signature.
  // Start by resetting it, to make it possible to call this function more
  // than once.
  signature = 0;
  
  for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
  {
    signature = ((signature << 1) ^ cur->m_size) ^ (!(signature & 0x8000));
    
    // The signature can never be 0 otherwise the code would think it's
    // not set yet
    if (!signature)
    {
      signature++;
    }
  }

  // If the list was empty, signature will still be 0 at this time.
  if (signature)
  {
    result = VerifySignature();

    if ((storealways) || ((!result) && (storeifinvalid)))
    {
      StoreAll();
      
      if (wipeunusedareas)
      {
        for (byte *u = (byte *)nextaddr + sizeof(signature); 
          u <= (byte *)E2END; u++)
        {
          if (eeprom_read_byte(u) != 0xFF)
          {
            eeprom_write_byte(u, 0xFF);
          }
        }
      }
    }  
    else if ((retrieveifvalid) && (result))
    {
      RetrieveAll();
    }
  }
  
  return result;
}


////////////////////////////////////////////////////////////////////////////
// END
////////////////////////////////////////////////////////////////////////////
