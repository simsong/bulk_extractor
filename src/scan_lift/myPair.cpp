#include "myPair.h"

//myPair.cpp
//Purpose: represents an indivdual dupple pair that contains is an immutable class that once
//	created it can not be changed.  Additionally, the data members are 
// 	const, this allows for read only access, therefore thread safe 
//	as well.


//feature(int, double)
//Effects: Sets and individual feature.  Once set it can no longer be unset, 
// 	since the class is immutable
myPair<T>::myPair(const T& A, const K& B)
	:pairA(A),
	 pairB(B)
{
}

//~feature()
//Effect: Default destructor
myPair<T>::~myPair() {}

//void getFeature(T&, K&)
//Effects: returns a reference to featureA, and to featureB 
void myPair<T>::getPair(const T& A, const K& B){
	A = pairA;
	B = pairB;
}

//bool isSet()
//Effects: ((A>0)&&(B>0)) = true othewise return false
bool myPair<T>::isMyPairSet(void){
	return pairSet;
}
