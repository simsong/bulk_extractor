// element.cpp
// Purpose implements a template  element.cpp

// Purpose a basic data element that can be used.  Provides thread saftey
// by wrapping a fundemental data type in a class by using a template to 
// pass the fundamental datatype to it.
//
// Also this element is thread safe by utilizing const as well as being 
// immutable - once set it can not be unset - the only accessor method
// is T getValue() which returns the element of type T

#include "element.h"

template <class T>
element<T>::element(T E){
   myElement = E;
}

template <class T>
T element<T>::getValue(void){
   return(myElement);
}
