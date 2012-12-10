#ifndef _MYPAIR_H
#define _MYPAIR_H 
// myPair.h
//Purpose: represents an indivdual dupple pair that contains an immutable class that once
//      created it can not be changed.  Additionally, the data members are 
//      const, this allows for read only access, therefore thread safe 
//      as well.

// Note: Assumption: I am assuming that A && B if set does not equal 0 this may not be a valid
// assumption.  Need to check it later though
 

template <class T, class K> 
class myPair {
   public:
//Effects: Sets and individual feature.  Once set it can no longer be unset, 
//      since the class is immutable
	myPair (const T& A, const K& B);

//Effects: returns a reference to pair element A, and to pair element B 
	void getValue(const T& A, const K& B);

//Effects: indicates that the pair is set
	bool isSet(void);

   private:
	const T pairA;
	const K pairB;
	const bool pairSet;
};
#endif
