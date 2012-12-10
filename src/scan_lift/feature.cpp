#include "feature.h"

//feature.cpp
//Purpose: represents a single feature that contains an immutable class that once
//	created it can not be changed.  Additionally, the data members are 
// 	const, this allows for read only access, therefore thread safe 
//	as well.


//feature(int, double)
//Effects: Sets and individual feature.  Once set it can no longer be unset, 
// 	since the class is immutable
feature::feature(int A, double B)
	:featureA(A),
	 featureB(B),
	 featSet(((A>0)&&(B>0))?true:false)
{
}

//~feature()
//Effect: Default destructor
feature::~feature() {}

//void getFeature(int&, double&)
//Effects: returns a reference to featureA, and to featureB 
void feature::getFeature(int& A, double& B){
	A = featureA;
	B = featureB;
}

//bool isSet()
//Effects: ((A>0)&&(B>0)) = true othewise return false
bool feature::isSet(void){
	return featSet;
}
