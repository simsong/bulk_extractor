#ifndef _FEATURE_H
#define _FEATURE_H

// feature.h
// Purpose: Definition of an individual freature used in the features

class feature{
   public:
	feature (int A, double B);
	~feature();
	void getFeature(int& featureA, double& featureB);
	bool isSet(void);

   private:
	const int featureA;
	const double featureB;
	const bool featSet;
};
#endif
