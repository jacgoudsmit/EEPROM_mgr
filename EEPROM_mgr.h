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
  
  In most cases, your sketch should retrieve all the stored values from the
  EEPROM, but the first time you do this (when the EEPROM isn't initialized
  yet), calling RetrieveAll will override your defaults with values that are
  probably invalid or useless. Here are some suggestions to handle this:
  
  1. You can temporarily modify the sketch to call StoreAll at the beginning
  of your sketch, and run this version once, before modifying it back and
  downloading it.
  
  2. You can write a separate sketch to store the default values. You can
  combine this with other functionality, e.g. a sketch to calibrate servo
  values could modify the values until they're just right, and then store
  them. Make sure that all declarations of items use the same types and
  are declared in the same order. Declarations may be spread across
  multiple modules and libraries but all the #includes should be in the
  same order in all modules to prevent EEPROM locations from changing
  between sketches.
  
  3. You can test an input at the beginning of execution, and if it has a
  certain value, you can store the defaults instead of retrieving the values
  from EEPROM. This way, for example, you can instruct the user to hold a
  pin low or keep a button pressed to return the settings to their defaults.
  
  4. Declare a byte item and use it to indicate whether the EEPROM has been
  initialized: at the beginning of your sketch, call Retrieve on the byte
  item and check if the value is 255 afterwards. If it is, set the byte
  to a different value (e.g. 0) and call StoreAll() to store all the values,
  otherwise call RetrieveAll (and optionally do a sanity check to make
  sure that what's in the EEPROM makes sense). You can store 255 in the
  byte and reset the Arduino to revert the settings to the defaults.
  
    
  The library doesn't attempt to do any wear-levelling. I started adding
  code to do this but it got so complicated that it took up much more 
  space than I wanted to use. Maybe in the future I'll write an advanced
  version that does wear-levelling but for my application it just wasn't
  enough of a concern. Even if the EEPROM fails, the cost to replace the
  entire chip is minimal. The library doesn't attempt to detect EEPROM
  failures but if your sketch needs this, it can call the Verify or
  VerifyAll function to make sure that what's in the items, is what's in the
  EEPROM. Obviously this probably is NOT the case right after startup, 
  because at that time, the defaults are loaded, not the values from the
  EEPROM.
*/

#include <avr/eeprom.h>


////////////////////////////////////////////////////////////////////////////
// Base class for EEPROM items
//
// It's not possible to create instances of this class, it's only needed for
// the basic functionality and the static members and functions.
class EEPROM_mgr
{
  //------------------------------------------------------------------------
  // Static variables
protected:
  static word nextaddr;
  static EEPROM_mgr *list;

  
  //------------------------------------------------------------------------
  // Member variables
protected:
  EEPROM_mgr *m_next;
  word m_addr;
  size_t m_size;

  
  //------------------------------------------------------------------------
  // Constructor/Destructor
public:
  EEPROM_mgr(size_t size)
  : m_addr(nextaddr)
  , m_size(size)
  {
    nextaddr += size;
    
    m_next = list;
    list = this;
  }
  
  virtual ~EEPROM_mgr()
  {
    // Normally this should never be called but this code will clean
    // everything up nicely.
    
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
  }


  //------------------------------------------------------------------------
  // Pure virtual function that provides a pointer to the RAM data
  // that's associated with this item
public:
  virtual void *Data() = 0;
  

  //------------------------------------------------------------------------
  // EEPROM operations per item
public:
  void Store()
  {
    eeprom_write_block(Data(), (void *)m_addr, m_size); 
  }
  
  void Retrieve()
  {
    eeprom_read_block(Data(), (const void *)m_addr, m_size);
  }
  
  bool Verify()
  {
    bool result = true;
    const byte *p = (const byte *)Data();
    const byte *e = (const byte *)m_addr;
    
    for (size_t n = 0; n < m_size; n++, e++)
    {
      if (eeprom_read_byte(e) != *p)
      {
        result = false;
        break;
      }
    }
    
    return result;
  }


  //------------------------------------------------------------------------
  // EEPROM operations for all items
  // These should be called as EEPROM_mgr::functionname();
public:
  static void StoreAll()
  {
    for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
    {
      cur->Store();
    }
  }
  
  static void RetrieveAll()
  {
    for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
    {
      cur->Retrieve();
    }
  }
  
  static void VerifyAll()
  {
    for (EEPROM_mgr *cur = list; cur; cur = cur->m_next)
    {
      cur->Verify();
    }
  }
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
// The library doesn't check that the values are retrieved from the correct
// locations; in other words, you will get invalid data if you store the
// values with a sketch that has the items declared in one way, and 
// retrieve them in a sketch that has the declarations in a different
// order or uses different variable types.
template <class T> class EEPROM_item : public EEPROM_mgr
{
  // The member that stores the actual data
protected:
  T m_data;

  // Constructor/Destructor
public:
  EEPROM_item()
  : EEPROM_mgr(sizeof(T))
  , m_data()
  {
  }
  
  EEPROM_item(const T& defaultvalue)
  : EEPROM_mgr(sizeof(T))
  , m_data(defaultvalue)
  {
  }

  // Virtual function that provides access to the data
public:
  virtual void *Data()
  {
    return &m_data;
  }

  // Functions for read and write access in the program
public:
  operator const T&()
  {
    return m_data;
  }
  
  const T& operator=(const T& src)
  {
    if (m_data != src)
    {
      m_data = src;
      Store();
    }
    return m_data;
  }
};
