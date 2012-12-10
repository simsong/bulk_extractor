#ifndef _LINEAR_OVA_SVM_H_
#define _LINEAR_OVA_SVM_H_

#include "myheader.h"
#include "base.h"
#include "linear_binary_svm.h"
#include <pthread.h>

/**
 * The linear One Versus All (OVA) Support Vector Machine.
 * The OVA runs each of the binary SVMs and picks the best.
 */

class LinearOvaSVM : public BaseClassifier {
private:
    /*** neither copying nor assignment is implemented                         ***
     *** We do this by making them private constructors that throw exceptions. ***/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    LinearOvaSVM( const LinearOvaSVM &l):
	BaseClassifier(l.config),classes(),class_id_to_Bcls_mapping(),Bcls(){
	throw new not_impl();
    };
    const LinearOvaSVM &operator=(const LinearOvaSVM &w){ throw new not_impl();}
public:
    struct linear_ova_config_t {
	const int class_id;
	const char *class_name;
	const int wt_max;
	const int positive;
	const int negative;
	const double threshold;
	double *weights;
    };

    vector<int> classes;
    map<int,int> class_id_to_Bcls_mapping;
    vector<LinearBinarySVM *> Bcls;

    /** Create a LinearOvaSVM for training */
    LinearOvaSVM( DataBundle& training , const ClassifierConfig& _config ):
	BaseClassifier(_config),classes(),class_id_to_Bcls_mapping(),Bcls(){
	re_train(training);
    }

    /** Create a LinearOvaSVM from a model file; the catalog must be previously loaded */
    LinearOvaSVM( const ClassifierConfig& _config , const string& model_fname ):
	BaseClassifier( _config ),classes(),class_id_to_Bcls_mapping(),Bcls(){
	load_textfile( model_fname );
    }

    /** Create a LinearOvaSVM from a precompiled description */
    LinearOvaSVM( const ClassifierConfig& _config , const struct linear_ova_config_t **ovas);

    /** Create an empty LinearOvaSVM */
    LinearOvaSVM( const ClassifierConfig& _config ): 
	BaseClassifier( _config ),classes(),class_id_to_Bcls_mapping(),Bcls(){
    }
    ~LinearOvaSVM(){
	EACH(it,Bcls)  delete (*it);
    }

    void clear();
    void re_train( DataBundle& ) ;
    void classify( DataBundle& ) const;
    void classify_document( Document& ) const;

    vector< pair<int,double> > get_classifier_score(const Document& d ) const;
    double get_classifier_score_for_class(const Document& d , int class_id ) const;

    void save( const string& model_fname ) const;
    void load_textfile( const string& model_fname );
    void load_precompiled( const struct LinearOvaSVM::linear_ova_config_t *ovas[]);
    void dump(std::ostream &os) const;
};


#endif
