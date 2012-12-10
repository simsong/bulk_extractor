#include "term_weighting.h"
#include "base.h"

void TermWeighting::clear()
{
    std::cerr << "*** TermWeighting::clear()\n";
    term_freq.clear();
    doc_freq.clear();
    if( term_freq_ar ){
	delete[] term_freq_ar;
	term_freq_ar = 0;
    }
    if( doc_freq_ar ){
	delete[] doc_freq_ar;
	doc_freq_ar = 0;
    }
 
    n_docs = use_array = n_features = -1;
}

void TermWeighting::re_train( const DataBundle& bundle )
{
    std::cerr <<" Term weighting : training on dataset = " << bundle.get_dataset_name() << "\n";
  
    this->clear();
    n_docs = bundle.size();
    n_features = bundle.nfeatures + 1;

    if( bundle.nfeatures < 5000000 ) {
   
	// Use arrays
	use_array = 1; 
	term_freq_ar = new double[ n_features ], doc_freq_ar = new double[ n_features ];
	REP(i,n_features) term_freq_ar[i] = doc_freq_ar[i] = 0;
	EACH(docs,bundle) EACH(f,docs->Features) {
	    term_freq_ar[ f->first ] += f->second;
	    if( f->second > EPS ) doc_freq_ar[ f->first ] += 1;
	}
    } else {
	// Use map
	use_array = 0;
	EACH(docs,bundle) EACH(f,docs->Features) {
	    term_freq[ f->first ] += f->second;
	    if( f->second > EPS ) doc_freq[ f->first ] += 1;
	}
    }
}

void TermWeighting::apply( const string& scheme , DataBundle& bundle ) {
    if( !( scheme == "ltc" || scheme == "nnc" ) ) {
	std::cerr <<" Only 'ltc' term weighting scheme supported \n";   exit(0);
    }
    if( n_docs <= 0 || n_features <= 0 ) {
	std::cerr <<" Cannot apply term-weighting before successful traiing \n"; exit(0);
    }
    std::cerr <<" Applying scheme = " << scheme <<" For term weighting .. \n";
    if( scheme == "ltc" ) {
	if( use_array ) {
	    double *log_n_doc_freq = new double[ n_features+1 ];
	    REP(i,n_features) if( doc_freq_ar[i] > EPS ) log_n_doc_freq[ i ] = log( n_docs *1.0/ doc_freq_ar[i] );
	    EACH(docs,bundle) EACH(f,docs->Features) f->second = ( 1 + log( f->second ) ) * log_n_doc_freq[ f->first ];
	    delete[] log_n_doc_freq;
	} else {
	    map<int,double> log_n_doc_freq;
 
	    EACH(it,doc_freq) log_n_doc_freq[ it->first ] = log( n_docs * 1.0 / it->second );
	    EACH(docs,bundle) EACH(f,docs->Features) f->second = ( 1 + log(  f->second ) ) * log_n_doc_freq[ f->first ];
	}
    }
    // Do cosine normalization
    EACH(docs,bundle) docs->normalize_to_unit_length();

}

TermWeighting::~TermWeighting() {
    if( term_freq_ar ) delete[] term_freq_ar;
    if( doc_freq_ar ) delete[] doc_freq_ar;
}


