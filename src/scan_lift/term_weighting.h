#ifndef _TERM_WEIGHTING_H_
#define _TERM_WEIGHTING_H_


#include "base.h"

// Currently supports only LTC term weighting

/**
 * Weights of terms of the support vector machine?
 */
class TermWeighting { 
private:
    /*** neither copying nor assignment is implemented                         ***
     *** We do this by making them private constructors that throw exceptions. ***/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying TermWeighting objects is not implemented.";
	}
    };
    TermWeighting(const TermWeighting &tw):term_freq(),doc_freq(),term_freq_ar(),doc_freq_ar(),
					   n_docs(),n_features(),use_array() {
	throw new not_impl();
    };
    const TermWeighting &operator=(const TermWeighting &tw){throw new not_impl();}
    
public:
    TermWeighting( ):
	term_freq(),doc_freq(),term_freq_ar(NULL),doc_freq_ar(NULL),
	n_docs(0),n_features(0),use_array(0){
    }
    TermWeighting( const DataBundle& d ): // create and re-train
	term_freq(),doc_freq(),term_freq_ar(NULL),doc_freq_ar(NULL),
	n_docs(0),n_features(0),use_array(0){
	re_train(d);
    }

    virtual ~TermWeighting();


    map<int,double> term_freq;		
    map<int,double> doc_freq;

    // Term freq array is used when the no of features is less than 5e6 
    
    double *term_freq_ar, *doc_freq_ar;
    int n_docs, n_features, use_array;
    
    
    void clear();
    void apply( const string& scheme , DataBundle& d );
    void re_train( const DataBundle& );
};



#endif
 
