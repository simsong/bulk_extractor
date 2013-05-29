#include <limits.h>	// for INT_MAX
#include "myheader.h"
#include "linear_binary_svm.h"
#include "linear_ova_svm.h"
#include "../bulk_extractor.h"

/*
 * File format:
 * -1:class - start of binary SVM #class
 * <positive>:<negative>
 * <threshold>
 * n:weights[n]
 * ...
 */


LinearOvaSVM::LinearOvaSVM( const ClassifierConfig& _config , const struct linear_ova_config_t **ovas):
    BaseClassifier( _config ),classes(),class_id_to_Bcls_mapping(),Bcls(){
    load_precompiled(ovas);
}


void LinearOvaSVM::clear(){
    std::cerr << "*** LinearOvaSVM::clear ** Bcls.clear() is called, causing " << Bcls.size() << " objects to leak\n";
    classes.clear();
    Bcls.clear();
    class_id_to_Bcls_mapping.clear();
}

void LinearOvaSVM::re_train( DataBundle& training ) {
    clear();
    std::cerr <<" Starting training for LinearOvaSVM Classifier \n";
    classes = training.classes();
    DataBundle new_bundle = training;
    if( classes.size() == 2 ) classes.pop_back(); // Just run svm for the first class in case two classes
    REPV(c,classes) {
	/* Create a new bundle for the class 'classes[c]' OVA */

	std::cerr <<" Training Binary Classifier for " << classes[c] <<" \n";
   
	double newC = config.LinearOvaSVM::classwise_C.count( classes[c] ) ?
	    config.LinearOvaSVM::classwise_C[ classes[c] ] : config.LinearBinarySVM::C;

	// No print if one classifier has been trained.
	BaseClassifier::config.LinearBinarySVM::set_print_dataset( Bcls.size() == 0 ); 
	BaseClassifier::config.LinearBinarySVM::set_class_id( classes[c]  );
	ClassifierConfig base_binary_classifier_config( BaseClassifier::config );
	base_binary_classifier_config.LinearBinarySVM::set_C( newC ); 
	LinearBinarySVM *B = new LinearBinarySVM( new_bundle , base_binary_classifier_config  );
	class_id_to_Bcls_mapping[ classes[c] ] = Bcls.size();
	Bcls.push_back( B );
    }
    std::cerr <<" LinearOvaSVM training complete \n";
}

void LinearOvaSVM::classify( DataBundle& testing ) const {
    EACH(it,testing.docs) classify_document( *it );
}

void LinearOvaSVM::classify_document( Document& d ) const {
    d.label_scores = get_classifier_score( d );
}

// Possibly remove function not called at all in identification
// instead LinearBinarySVM called instead 
double LinearOvaSVM::get_classifier_score_for_class(const Document& d , int class_id ) const { 

    if( class_id_to_Bcls_mapping.count( class_id ) ) {
	int bcls_no = (class_id_to_Bcls_mapping.find( class_id )->second);
	double result = Bcls[ bcls_no ]->get_classifier_score_for_class( d , +1 );
	return result;
    }
    std::cerr <<" Illegal Class specification .. \n";
    exit(0);
}
//------------------------------------------------------------------------------------
#define not_possible( x ) ( abs( x - INT_MAX ) < EPS )
vector< pair<int,double> > LinearOvaSVM::get_classifier_score(const Document& d ) const {
    vector< pair<int,double> > V;
    REPV(c,classes) {
	double score = Bcls[ c ]->get_classifier_score_for_class( d , +1 );
	if( not_possible( score ) ) continue;
	V.push_back( make_pair( classes[ c ] , score ) );
    }
    return V;
}

/**
 * Load from an ascii file
 */

void LinearOvaSVM::load_textfile( const string& fname )
{
    FILE *f = fopen( fname.c_str() , "r" ); assert( f );
    int class_id = 0;
    int fid = 0;
    double fval=0;
    LinearBinarySVM *B = NULL;
    while( fscanf(f,"%d:%lf",&fid,&fval) == 2 ) { 
	if( fid == -1 ) {
	    /* Create new scanner class fval */
	    class_id = int(fval);
	
	    B = new LinearBinarySVM( BaseClassifier::config  );
	    B->alloc_weights();
	    if(fscanf(f,"%d:%d",&B->positive,&B->negative) != 2){
		errx(1,"linear_ova_svm.cpp: file format error 1\n");
	    }
	    if(fscanf(f,"%lf",&B->threshold) !=1){
		errx(1,"linear_ova_svm.cpp: file format error 2\n");
	    }
	    classes.push_back( class_id );
	    class_id_to_Bcls_mapping[ class_id ] = Bcls.size();
	    Bcls.push_back( B );
	    continue;
	}
	if(fid < 0){
	    errx(1,"linear_ova_svm.cpp: Invalid file format error 3\n");
	}
	B->weights[ fid ] = fval;
	if((unsigned)fid > B->wt_max) B->wt_max = fid;
    }
    fclose( f );
}

/**
 * Load from a precompiled model.
 */
void LinearOvaSVM::load_precompiled(const struct linear_ova_config_t *ovas[])
{
    for(size_t i=0;ovas[i];i++){
	const struct linear_ova_config_t &ova = *ovas[i];
	LinearBinarySVM *B = new LinearBinarySVM( BaseClassifier::config );
	B->positive	= ova.positive;
	B->negative	= ova.negative;
	B->threshold	= ova.threshold;
	classes.push_back( ova.class_id);
	class_id_to_Bcls_mapping[ ova.class_id ] = Bcls.size();
	B->weights	= ova.weights;
	B->should_free_weights = false;
	B->wt_max	= ova.wt_max;
	Bcls.push_back( B );
    }
}

/**
 * Write the OVA SVM to a text file.
 * The format consists of each binary SVM, one after the other.
 */
void LinearOvaSVM::save( const string& fname ) const
{
    FILE *f = fopen( fname.c_str() , "wb" ); assert( f );
    REPV(i,Bcls){
	fprintf(f,"-1:%d\n", classes[i] );
	fprintf(f,"%d:%d\n", Bcls[i]->positive, Bcls[i]->negative);
	fprintf(f,"%.9lf\n",Bcls[i]->threshold);
	for(size_t j = 0;j< Bcls[i]->wt_capacity; j++){
	    if( abs(Bcls[i]->weights[j]) > EPS ){
		fprintf(f,"%zu:%.9lf\n",j,Bcls[i]->weights[j]);
	    }
	}
    }
    fclose( f );
    std::cerr <<" Save complete \n";
}

void LinearOvaSVM::dump(std::ostream &os) const 
{
    os << "LinearOvaSVM:\n";
    os << "  classes: ";   EACH(it,classes) os << (*it) << " "; os << "\n";
    os << "class_id_to_Bcls_mapping:\n";
    EACH(it,class_id_to_Bcls_mapping) os << " class_id_to_Bcls_mapping[" << it->first << "]=" << it->second << "\n";
    EACH(it,Bcls) (*it)->dump(os);
    os << "===\n";
}
