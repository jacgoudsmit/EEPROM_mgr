/*
  EEPROM Manager Library
  ----------------------
  By Jac Goudsmit
  Distributed under the BSD (3-clause) license.
  http://github.com/jacgoudsmit/EEPROM_mgr
  
  This library allows you to store data on the EEPROM of the Arduino without
  worrying too much about where the data goes, when to update the EEPROM, 
  when to retrieve the data from EEPROM etc. All you need to do is declare 
  any number of EEPROM_item<type> variables. From then on, you can treat 
  each item as its type: you can cast the item to its type, and you can
  assign the item from a variable that has the same type.
  
  IMPORTANT: YOU SHOULD DECLARE ITEMS OUTSIDE FUNCTIONS ONLY. If you would
  declare an item inside a function, a new item (with a different location
  in the EEPROM) would be created each time the function is called. Surely
  that's not what you want :-).

  It's possible to declare a default value for an item by using a parameter
  in the constructor. When the setup() function of your sketch starts, the
  default values are stored in the items. At that point, you need to decide
  whether to retrieve the values from the EEPROM (using the static 
  RetrieveAll function) or to store all the default values in the EEPROM
  (using StoreAll). 
  
  In most cases, your sketch should start by calling the static Begin
  function to retrieve values from the EEPROM. The library generates a
  signature value in order to make sure (to some extent) that the data is
  valid.
  
  The library does some minimal wear-prevention by trying not to overwrite
  values if the EEPROM doesn't need to be changed, but it doesn't do any
  real wear-leveling. I started adding it but it got so complicated that
  it took up much more space than I wanted to use. Maybe in the future I'll
  write an specialized advanced version that does wear-leveling but for
  most needs it just isn't enough of a concern. Even if the EEPROM fails,
  the cost to replace the entire chip is minimal.

  The library doesn't attempt to detect EEPROM failures but if your sketch
  needs this, it can call the Verify or VerifyAll function to make sure
  that the EEPROM contains the same data as the items, after they are
  supposed to be written.
*/


#ifndef EEPROM_MGR_H
#define EEPROM_MGR_H

#include <Arduino.h>


//--------------------------------------------------------------------------
// Implementation for the "missing" EEPROM function
bool                                    // Returns true if all data matches
eeprom_verify_block(
  const void *ram_data,                 // Pointer to RAM data
  void *eeprom_data,                    // Pointer to EEPROM data
  size_t size);                         // Number of bytes to compare


////////////////////////////////////////////////////////////////////////////
// Base class for EEPROM items
////////////////////////////////////////////////////////////////////////////
//
// It's not possible to create instances of this class, it's only needed for
// the basic functionality and the static members and functions. Use the
// template class below to create items that are stored in the EEPROM.
class EEPROM_mgr
{
  //------------------------------------------------------------------------
  // Static variables
protected:
  static byte      *nextaddr;           // Next address in EEPROM
  static EEPROM_mgr *list;              // List of items with nonzero size
  static word       signature;          // non-zero=list is finalized

  
  //------------------------------------------------------------------------
  // Member variables
protected:
  EEPROM_mgr       *m_next;             // Next link in linked list
  byte             *m_addr;             // NOTE: 0 is valid EEPROM address!
  size_t            m_size;             // Size of data; 0=don't use EEPROM

  
  //------------------------------------------------------------------------
  // Constructor
public:
  EEPROM_mgr(
    size_t size);                       // Size for data required by item
  

  //------------------------------------------------------------------------
  // Destructor
public:
  virtual ~EEPROM_mgr();

  
  //------------------------------------------------------------------------
  // Prevent creation on the heap by declaring the new operator as protected
protected:
  void *operator new(size_t);
  
  
  //------------------------------------------------------------------------
  // Pure virtual function that provides a pointer to the RAM data
  // that's associated with this item
public:
  virtual void *Data() = 0;
  

  //------------------------------------------------------------------------
  // Store the item into the EEPROM
  //
  // Protected because there's no check if the list is finalized
protected:
  void _Store();
  
  
  //------------------------------------------------------------------------
  // Store after checking signature
public:
  void Store()
  {
    if (signature)
    {
      _Store();
    }
  }
  
  
  //------------------------------------------------------------------------
  // Retrieve the item from the EEPROM
  //
  // Protected because there's no check if the list is finalized
protected:
  void _Retrieve();
  
  
  //------------------------------------------------------------------------
  // Retrieve after checking signature
public:
  void Retrieve()
  {
    if (signature)
    {
      _Retrieve();
    }
  }
  
  
  //------------------------------------------------------------------------
  // Verify if the value in the item matches the value stored in EEPROM
  //
  // Protected because there's no check if the list is finalized
protected:
  bool _Verify();

  
  //------------------------------------------------------------------------
  // Verify after checking signature
public:
  bool Verify()
  {
    bool result = false;

    if (signature)
    {
      result = _Verify();
    }

    return result;
  }
  
  
  //------------------------------------------------------------------------
  // Static helper function to check the signature in the EEPROM matches
public:
  bool                                  // Returns true if EEPROM sig valid
  static VerifySignature(void);
  
  
  //------------------------------------------------------------------------
  // Static function to save all values and write the signature
public:
  static void StoreAll(
    bool forcewritesig = false);
  
  
  //------------------------------------------------------------------------
  // Retrieve all values but only if the signature is correct
public:
  static bool                           // Returns true if values retrieved
  RetrieveAll();
  
  
  //------------------------------------------------------------------------
  // Verify that all values in the EEPROM are equal to the stored values
public:
  static bool                           // Returns true if all values match
  VerifyAll();
  
  
  //------------------------------------------------------------------------
  // This should be called at the beginning of your sketch
  //
  // This finalizes the list of items: no more items can be declared.
  // Since all items should be statically declared, this shouldn't be a
  // problem anyway.
  //
  // It also either (A) retrieves all items from the EEPROM if the signature
  // in the EEPROM matches the signature generated from the list of items,
  // or (B) stores all the current (default) values, and the new signature
  // in the EEPROM and clears the rest of the EEPROM.
  //
  // You can override this behavior via the parameters:
  // - If you want to leave the EEPROM alone if the signature is not there,
  //   you can override the first parameter. This can be used if your app
  //   has to work together with other applications that use the EEPROM,
  //   and you have code in your application that somehow decides which
  //   application has control over what goes into the EEPROM.
  // - If you always want to overwrite the EEPROM with the default values
  //   even if the signature is found, you can use the second parameter.
  //   This may be useful during debugging, when you change the default
  //   values but not the sizes, and you want to make sure that the new 
  //   values get written
  // - If you don't want to clear the unused part of the EEPROM, you can
  //   use the third parameter. This can be useful if you're using multiple
  //   applications or when you're storing data into the EEPROM yourself.
  // - If you only want to calculate the signature and check if the EEPROM
  //   has a valid signature, use the fourth parameter.
  //    
  // The return value indicates whether the EEPROM has a valid signature.
public:
  static bool 
  Begin(
    bool storeifinvalid = true,
    bool storealways = false,
    bool wipeunusedareas = true,
    bool retrieveifvalid = true);
};


////////////////////////////////////////////////////////////////////////////
// EEPROM item template class
//
// You can declare any number of items, in any number of modules as static
// or global variables. The base class (above) automatically keeps track of
// the EEPROM address of each item, based on the size of each item.
//
// To keep everything as simple as possible, the EEPROM location of each
// item depends on the order in which they are declared. This can be
// somewhat problematic when the same EEPROM locations are needed by
// multiple sketches, but this can be alleviated by declaring all the items
// in one library and including that library by all sketches that need it.
template <class T> class EEPROM_item : public EEPROM_mgr
{
  //------------------------------------------------------------------------
  // Member variables
public:
  T                 m_data;             // The actual data being stored

  
  //------------------------------------------------------------------------
  // Constructor
public:
  EEPROM_item()
  : EEPROM_mgr(sizeof(T))
  , m_data()
  {
  }
  
  
  //------------------------------------------------------------------------
  // Destructor
public:
  EEPROM_item(const T& defaultvalue)
  : EEPROM_mgr(sizeof(T))
  , m_data(defaultvalue)
  {
  }

  
  //------------------------------------------------------------------------
  // Virtual function that provides access to the data
public:
  virtual void *Data()
  {
    return &m_data;
  }
};


////////////////////////////////////////////////////////////////////////////
// END
////////////////////////////////////////////////////////////////////////////

#endif
