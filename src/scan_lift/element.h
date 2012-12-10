#ifndef _ELEMENT_H
#define _ELEMENT_H

// element.h
// Purpose a basic data element that can be used.  Provides saftey
// by wrapping a fundemental data type in a class by using a template to 
// pass the fundamental datatype to it.
//
// Also this element is thread safe by utilizing const as well as being 
// immutable - once set it can not be unset - the only accessor method
// is T getValue() which returns the element of type T

template <class T>
class element{
   public:
// Effects: Essentially sets the element to a type as specified in the 
// template - Once set it can not be unset.
// Ex. if T is type int then myelement is of type const int
   element(T E);

// Effects: Returns myElement of type T   
   T getValue(void);

   private:
   const T myElement;
};
#endif

