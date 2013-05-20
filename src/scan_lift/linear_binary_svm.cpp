#include <stdio.h>	// for memset
#include <string.h>	// for memset
#include <limits.h>	// for INT_MAX
#include "base.h"
#include "linear_binary_svm.h"
#include "lift_utils.h"
#include <errno.h>
//#include "../md5.h"

static std::string stringPrintf( const char *Format, ... )
{
    /* does an sprintf of its remaining arguments according to Format,
       returning the formatted result. */
    std::string Result;
    char * TempBuf;
    int TempBufSize;
    TempBuf = NULL;
    TempBufSize = 1024; /* something reasonable to begin with */

    for (;;){
	TempBuf = (char *)malloc(TempBufSize);

	if (TempBuf == NULL){	// out of memory; early return
	    break;
	}

	va_list Args;
	va_start(Args, Format);
	const long BufSizeNeeded = vsnprintf(TempBuf, TempBufSize, Format, Args);

	/* Not including trailing null */
	va_end(Args);
	if (BufSizeNeeded >= 0 and BufSizeNeeded < TempBufSize){
	    /* The whole thing did fit */
	    TempBufSize = BufSizeNeeded; /* not including trailing null */
	    break;
	}

	free(TempBuf);

	if (BufSizeNeeded < 0){
	    /* glibc < 2.1 */
	    TempBufSize *= 2; /* try something bigger */
	}         
	else  {
	    /* glibc >= 2.1 */
	    TempBufSize = BufSizeNeeded + 1;
	    /* now I know how much I need, including trailing null */
	}
    }
    if (TempBuf != NULL){
	Result.append(TempBuf, TempBufSize);
	free(TempBuf);
    }
    free(TempBuf);
    return Result;
}




void LinearBinarySVM::configure()
{
    svm_learn_exe = Config::get_value( "linear_binary_svm_svm_learn_exe");
    svm_perf_learn_exe = Config::get_value("linear_binary_svm_svm_perf_learn_exe");
    model_file_base = Config::get_value( "linear_binary_svm_model_file_base");
    input_file_base = Config::get_value( "linear_binary_svm_input_file_base");
    weight_file_base = Config::get_value( "linear_binary_svm_weight_file_base");
    model_to_weight_converter = Config::get_value( "linear_binary_svm_model_to_weight_converter");
    /*
      if( svm_learn_exe.size() == 0 || model_file_base.size() == 0 || input_file_base.size() == 0 || model_to_weight_converter.size() == 0 ) {
      LOG <<" LinearBinarySVM CONFIG RECEIVED IMPROPER PARAMETERS .. \n";
      exit(0);
      }
    */
}

/* TM: Rewrote the constructor for LinearBinary svm to perform member initialization */
/* TM: Setting postive, negative to 0 - not sure if that is correct */ 

/* Reset the weights to zero and refresh the Binary Classifier */
void LinearBinarySVM::clear( )
{
    std::cerr <<"clear\n";
    threshold = positive = negative = 0;
    if(weights) memset(weights,0,sizeof(weights[0])*wt_capacity);
}

void LinearBinarySVM::re_train( DataBundle& training )
{
    clear();
    std::cerr <<" scan_lift/linear_binary_svn.cpp: Training Linear Binary SVM .. ! \n";
    string input_file  = stringPrintf("%s-%s-svm", input_file_base.c_str() , training.get_dataset_name().c_str() ); 
    string model_file  = stringPrintf("%s-%s-svm", model_file_base.c_str() , training.get_dataset_name().c_str() );
    string weight_file = stringPrintf("%s-%s-svm", weight_file_base.c_str() , training.get_dataset_name().c_str() );
    // There should be atleast one positive and one negative example
    positive = 0, negative = 0;
    int class_id = BaseClassifier::config.LinearBinarySVM::class_id;
    EACH(it,training) EACH(c,it->labels) positive += ( *c == class_id ), negative += ( (*c) != class_id );
 
    // Print training file 
    std::cerr <<" scan_lift/linear_binary.svn: Opening file : " << input_file << " to write training examples \n";
    if( config.LinearBinarySVM::print_dataset == 1 )  {
	FILE *input_fpath_fptr = fopen( input_file.c_str() , "wb" );
    
	assert( input_fpath_fptr );
	training.print( input_fpath_fptr );
	fclose( input_fpath_fptr );
    }

    // Do the learning 
    string command;
    if( config.LinearBinarySVM::is_svm_perf == 0 ) {
	command = stringPrintf("%s -G %d -h 100 -n 0 -c %lf %s %s > /dev/null 2> /dev/null",
			       svm_learn_exe.c_str() ,
			       config.LinearBinarySVM::class_id ,
			       config.LinearBinarySVM::C , input_file.c_str() , model_file.c_str() );
    }
    else {
	command = stringPrintf("%s -l %d -w 3 -h 100 -n 0 -c %lf %s %s > /dev/null 2> /dev/null",
			       svm_perf_learn_exe.c_str() ,
			       config.LinearBinarySVM::loss_function, config.LinearBinarySVM::C , input_file.c_str() , model_file.c_str() );
    }
    std::cerr <<" Running command = " << command << "\n";
    if(system( command.c_str() )){
	std::cerr << " ** error ** \n";
    }

    // Extract the Weights 
    command = stringPrintf("perl %s %s > %s",model_to_weight_converter.c_str() , model_file.c_str() , weight_file.c_str() );
    std::cerr <<" Extracting weights command-line = " << command << "\n";
    if(system( command.c_str() )){
	std::cerr << " *** error ** \n";
    }

    // Extract the weights from the model 
    FILE *f = fopen( weight_file.c_str() , "rb" );
    if(!f){
	std::cerr << "Cannot open " << weight_file << ":" << strerror(errno) << "\n";
	exit(1);
    }
  
    // Parse all the info and store it 
    unsigned int a;
    double x;
    threshold = 0;
    if(fscanf( f , "%u:%lf", &a , &threshold )!=2){
	std::cerr << "invalid file format\n";
    }
    while( fscanf(f , "%u:%lf", &a , &x ) == 2 ){
	assert( a < wt_capacity);
	weights[ a ] = x;
	if(a>wt_max) wt_max = a;
    }
    fclose( f );
    std::cerr <<" Extracted Weights \n ";
}

/* Return the scores of the new test documents */
void LinearBinarySVM::classify( DataBundle& testing ) const
{
    std::cerr <<" Running binary classifier on the test-set [ " << testing.docs.size() << " documents ] ... \n";
 
    vector< double > scores;
    EACH(it,testing) classify_document( *it );
    std::cerr <<" Classification of testing bundle complete on binary classifier \n";
}

/* Just add a single label score   */
void LinearBinarySVM::classify_document(Document& d ) const 
{
    d.label_scores = get_classifier_score( d );
}

/** Try to classify a single document and add the same
 */
double LinearBinarySVM::get_classifier_score_for_class(const Document& d , int class_id ) const
{
    if( !( positive > 0 && negative > 0 ) ) return INT_MAX; // something is wrong
    double s = -threshold;

    /* Any feature number greater than nfeatures does not occur in the training set, therefore mapped to zero
     */
    EACH(iter,d){
	if( iter->first>0 && (unsigned)iter->first < wt_capacity ) {
	    s += weights[ iter->first ] * iter->second;
	}
    }
    return class_id > 0 ? s : -s;
}

vector< pair<int,double> > LinearBinarySVM::get_classifier_score(const Document& d ) const {
    cout << " LinearBinarySVM::get_classifier_score( " << endl;
    double s = -threshold;

    // Any feature number greater than nfeatures does not occur in the training set, therefore mapped to zero 
    EACH(iter,d){
	if( iter->first > 0 && (unsigned)iter->first < wt_capacity ){
	    s += weights[ iter->first ] * iter->second;
	}
    }
    vector< pair<int,double> > ret;
    if( positive > 0 && negative > 0 ) ret.push_back( make_pair( +1 , s ) ), ret.push_back( make_pair( -1 , s ) );
    return ret;
}

/**
 * Print information about the class
 */
void LinearBinarySVM::dump(std::ostream &os) const
{
    os << "wt_capcity=" << wt_capacity << " positive=" << positive << " negative=" << negative << " threadhold=" << threshold;
    /* Calculate the MD5 of the weights */
    //  os << "MD(weights) = " << md5_generator::hash_buf((uint8_t*)weights,sizeof(float)*wt_capacity).hexdigest()
    os << "\n";
}
