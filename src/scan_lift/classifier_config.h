#ifndef _CLASSIFIER_CONFIG_H_
#define _CLASSIFIER_CONFIG_H_

#include "myheader.h"
#include "rawheaders.h"

const int MAX_FEATURES = 16777220;
//const int MAX_LABELS = 15000;
//const int MAX_DOCS = 50000;


namespace classifier_config {

// TM: Used to be struct LinearBinarySVM{.....}
// TM: Made everything public for now
    class LinearBinarySVM {
    public:
	/* The following inline function acted as a constructor for the structure - converted it
	   into a constructor
	   inline LinearBinarySVM() {
	   is_svm_perf = 0, C = 0, max_features = MAX_FEATURES, loss_function = 1, print_dataset = 1, class_id = +1;
	   }
	*/
	/* Normally I would separate it out into an implementation file if given time separate out an implementation
	   for each class definition */
	LinearBinarySVM()
	    : is_svm_perf( 0 ),
	      print_dataset( 1 ),
	      loss_function ( 1 ),
	      max_features( MAX_FEATURES),
	      class_id( +1 ),
	      C( 0 )
	{}
   
	virtual  ~LinearBinarySVM(){}
	virtual void set_C( double x ) { C = x; }
	virtual void set_print_dataset(bool b ) { print_dataset = b; }
	virtual void set_class_id( int c ) { class_id = c; }
 
//TM: Keeping K public since I don't know if it is accessed later 
	bool is_svm_perf, print_dataset;
	int loss_function , max_features, class_id;
	double C;
    };

    class LinearOvaSVM : public virtual LinearBinarySVM {
    public:
	LinearOvaSVM():LinearBinarySVM(),classwise_C(){}
	virtual ~LinearOvaSVM(){}
	map<int,double> classwise_C;
    };


    class KnnClassifier {
    public:
	KnnClassifier()
	    : K( 10 )
	{}
	virtual ~KnnClassifier() {}

	//TM: Keeping K public since I don't know if it is accessed later 
	int K;
    };

    class LinearBinaryPerceptron {
    public: 
	LinearBinaryPerceptron()
	    : iterations( 1 ),
	      max_features( MAX_FEATURES )
	{}
	virtual ~LinearBinaryPerceptron() {}
	int iterations, max_features;	
/* TM: Previously defined as: 
   int iterations, max_features;
   inline LinearBinaryPerceptron() {
   iterations = 1, max_features = MAX_FEATURES;
   }
*/
    };

    class NonparametricKernelClassifier {
    public: 
	NonparametricKernelClassifier()
	    : bandwidth( 0.05 )
	{}
	virtual ~NonparametricKernelClassifier() {}
// TM: Public for now may make it private
	double bandwidth;

/* TM: Used to be defined as:
   double bandwidth;
   inline NonparametricKernelClassifier() : bandwidth(.05) {}
**/
    };

// TM: Used to be struct {.....}
// TM: Made everything public for now
    class ConstraintSVM { 
    public:
	ConstraintSVM()
	    : C( 0 ),
	      max_features( MAX_FEATURES )
	      /*, max_labels( MAX_LABELS )*/
	{}
 
	virtual ~ConstraintSVM() {}

	// TM: Public for now may make it private
	double C;
	int max_features /* , max_labels */;


/* TM: Previoius definition
   double C;
   int max_features, max_labels;
   inline ConstraintSVM() {
   C = 0, max_features = MAX_FEATURES, max_labels = MAX_LABELS; 
   }
**/
    };

// TM: Used to be struct {.....}
// TM: Made everything public for now
    class RanksvmClassifier { 
    public: 
	RanksvmClassifier() 
	    : C( 0 ),
	      print_dataset( true ),
	      max_features( MAX_FEATURES )
	{}
   
	virtual ~RanksvmClassifier() {}

	void set_print_dataset( bool b ) { print_dataset = b; } 

// TM: values public for now 
 
	double C;
	bool print_dataset;
	int max_features;

/* TM: Previous Definition
   double C;
   bool print_dataset;
   int max_features;
   inline RanksvmClassifier() { 
   C = 0, max_features = MAX_FEATURES, print_dataset = true; 
   }
   inline void set_print_dataset( bool b ) { print_dataset = b; }
****/
    };

// TM: Used to be struct {.....}
// TM: Made everything public for now
    class SVMMAPClassifier {
    public:
	SVMMAPClassifier()
	    : C( 0 ),
	      print_dataset( true ),
	      max_features( MAX_FEATURES )
	{}

	virtual ~SVMMAPClassifier() {}

	void set_print_dataset( bool b ) { print_dataset = b; }

// TM: values public for now 

	double C;
	bool print_dataset;
	int max_features;

/* TM: Previous definition 
   double C;
   bool print_dataset;
   int max_features;
   inline SVMMAPClassifier() {
   C = 0, max_features = MAX_FEATURES, print_dataset = true; 
   }
   inline void set_print_dataset( bool b ) { print_dataset = b; }
**/
    };

// TM: Used to be struct {.....}
// TM: Made everything public for now
    class LinearOvaMMP {
    public: 
	LinearOvaMMP()
	    : loss( 0 ),
	      max_features( MAX_FEATURES ) /* ,
					      max_labels( MAX_LABELS )*/
	{}
   
	virtual ~LinearOvaMMP() {}

// TM: values public for now 
	int loss, max_features /*, max_labels */;

/* TM: Previous Definition
   int loss , max_features, max_labels;
   inline LinearOvaMMP() {
   loss = 0, max_features= MAX_FEATURES, max_labels = MAX_LABELS;
   }
*/
    };
} // TM: End of all base class definition

//TM: ClassifierConfig is a child of the following previoulsy defined classes
//TM: Made everything public for now
class ClassifierConfig : public virtual classifier_config::LinearOvaSVM, 
			 public virtual classifier_config::LinearBinarySVM, 
			 public virtual classifier_config::KnnClassifier, 
			 public virtual classifier_config::LinearBinaryPerceptron, 
			 public virtual classifier_config::NonparametricKernelClassifier , 
			 public virtual classifier_config::ConstraintSVM , 
			 public virtual classifier_config::RanksvmClassifier , 
			 public virtual classifier_config::SVMMAPClassifier , 
			 public virtual classifier_config::LinearOvaMMP { 
public:
    inline void set_regularization( double C ) {
	SVMMAPClassifier::C = C;
	RanksvmClassifier::C = C;
	ConstraintSVM::C = C;
	LinearBinarySVM::C = C;
    }

    inline void set_bandwidth( double C ) {
	NonparametricKernelClassifier::bandwidth = C;
    }

    inline void set_classwise_regularization( const map<int,double>& C ) {
	LinearOvaSVM::classwise_C = C;
    }

    inline void set_print( bool b ) {
	LinearBinarySVM::set_print_dataset(b);
	RanksvmClassifier::set_print_dataset(b);
	SVMMAPClassifier::set_print_dataset(b);
    }
};

#endif




