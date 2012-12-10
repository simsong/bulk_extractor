#ifndef _LINEAR_BINARY_SVM_H_
#define _LINEAR_BINARY_SVM_H_

#include "base.h"
#include "lift_utils.h"
#include "lift_config.h"

/**
 * The binary recognizer. This determines if the document matches a model.
 */
class LinearBinarySVM : public BaseClassifier {
    /*** neither copying nor assignment is implemented                         ***
     *** We do this by making them private constructors that throw exceptions. ***/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    LinearBinarySVM( const LinearBinarySVM &l):BaseClassifier(l.config),
					       svm_learn_exe(),svm_perf_learn_exe(),model_file_base(),input_file_base(),
					       model_to_weight_converter(),weight_file_base(),wt_capacity(),wt_max(),positive(),negative(),
					       threshold(),weights(),should_free_weights(false){ throw new not_impl();}
    const LinearBinarySVM &operator=(const LinearBinarySVM &w){ throw new not_impl();}
public:
    /**
     * constructor for decision making
     */
    LinearBinarySVM( const ClassifierConfig& _config ) :
	BaseClassifier( _config), 
	svm_learn_exe( Config::get_value( "linear_binary_svm_svm_learn_exe") ),
	svm_perf_learn_exe ( Config::get_value("linear_binary_svm_svm_perf_learn_exe") ),
	model_file_base( Config::get_value( "linear_binary_svm_model_file_base") ),
	input_file_base ( Config::get_value( "linear_binary_svm_input_file_base") ),
	model_to_weight_converter( Config::get_value( "linear_binary_svm_model_to_weight_converter") ),
	weight_file_base( Config::get_value( "linear_binary_svm_weight_file_base") ),
	wt_capacity ( config.LinearBinarySVM::max_features),
	wt_max(0),
	positive(0), negative(0), threshold(0),weights(0),should_free_weights(false) {
    };
    /**
     * constructor for training.
     */
    LinearBinarySVM( DataBundle& training , const ClassifierConfig& _config ) :
	BaseClassifier( _config),
	svm_learn_exe( Config::get_value( "linear_binary_svm_svm_learn_exe") ),
	svm_perf_learn_exe ( Config::get_value("linear_binary_svm_svm_perf_learn_exe") ),
	model_file_base( Config::get_value( "linear_binary_svm_model_file_base") ),
	input_file_base ( Config::get_value( "linear_binary_svm_input_file_base") ),
	model_to_weight_converter( Config::get_value( "linear_binary_svm_model_to_weight_converter") ),
	weight_file_base( Config::get_value( "linear_binary_svm_weight_file_base") ),
	wt_capacity ( config.LinearBinarySVM::max_features),
	wt_max(0),
	positive(0), negative(0), threshold(0),weights(0),should_free_weights(false) {
    };
    virtual ~LinearBinarySVM() {
	if(should_free_weights) delete[] weights;
    };
    void alloc_weights(){
	if(!weights){
	    weights = new double[ wt_capacity ];
	    should_free_weights = true;
	}
    }

    void clear();
    void configure();
    void re_train( DataBundle& training );
    void classify( DataBundle& testing ) const;
    void classify_document(Document& d ) const;

    /**
     * @param d - the document being classified.
     * @return - Returns a vector with a single value:
     *           <+1, score> if positive>0 && negative>0
     *           <-1, score) if positive<=0 || negative<=0
     *
     */

    vector< pair<int,double> > get_classifier_score(const Document& d ) const; /* maps classifier # to score */

    /**
     * @param d - the set of ngrams being examined.
     * @param class_id - This is always +1 for classification; when is it -1?
     *                   Results are inverted if class_id<0.
     */
    double get_classifier_score_for_class( const Document& d , int class_id ) const;

    // Just return the score of the positive document 
    string	svm_learn_exe;
    string	svm_perf_learn_exe;
    string	model_file_base;
    string	input_file_base;
    string	model_to_weight_converter;
    string	weight_file_base;
    size_t	wt_capacity;		// currently 2^24+4???
    size_t	wt_max;			// highest number we have read
    int		positive;
    int		negative;
    double	threshold;
    double	*weights;			// points to the array of weights
    bool	should_free_weights;			// true if we should free weights
    void dump(std::ostream &os) const;
};

#endif
