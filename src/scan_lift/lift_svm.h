#ifndef LIFT_SVM_H
#define LIFT_SVM_H

/**
 * lift_svm class is used to create a singleton lift_svm object which
 * is the interface between bulk_extractor and LIFT.
 */

#include "../bulk_extractor.h"
#include "../sbuf.h"
#include "base.h"
#include "term_weighting.h"
#include "config.h"
#include "linear_ova_svm.h"
#include "lift_utils.h"
#include "classifier_config.h" 
#include "string.h"
#include "gflags.h"
#include "rawdefines.h"
#include "element.h"

typedef element<int> id;
typedef element<double> score;
typedef vector< pair<int,double> > feature;

class lift_svm {
private:
    /*** neither copying nor assignment is implemented                         ***
     *** We do this by making them private constructors that throw exceptions. ***/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    lift_svm(const lift_svm &l):ngram(0),classNames(),recognizer(){
	throw new not_impl();
    };
    const lift_svm &operator=(const lift_svm &w){ throw new not_impl();}

public:
    lift_svm(void):ngram( 2 ),classNames(),recognizer(){
    }

    virtual ~lift_svm(void);

    void   load_model_dir(const std::string &model_dir);
    void   load_pretrained();
    void   save_cpp(const std::string &outfile);
    string classify(const sbuf_t &sbuf,double *score) const; // classify and return a string of the doc type
    void   dump(std::ostream &os) const;

private:
    feature create_ngram(const sbuf_t &sbuf) const;    // Run the Classification Algorithms 
    typedef map<int,string> classNames_map_t;

    // Instance variables
    int    ngram;			// ngram length will be classified; 2 or 3
    classNames_map_t classNames; 	// maps classID to a type
    const class LinearOvaSVM *recognizer;	// a classifier of classifiersa

public:;
    static const struct LinearOvaSVM::linear_ova_config_t *ovas[];
    struct classnames_t {
	int class_id;
	const char *name;
    };
};

#endif
