#include "lift_svm.h"

//-----  Initialization Algorithms -------

lift_svm::~lift_svm(void)
{
    if(recognizer)     delete recognizer;
};

/**
 * Loads the catalog and the model from a directory
 * @param model_dir - the directory with the catalog and the model
 */
void lift_svm::load_model_dir(const std::string &model_dir)
{
    // step 1 - initialize the catalog
    std::string fn_cat_map(model_dir + "/cat_map");
    std::string fn_modelfile(model_dir + "/model");
    std::ifstream f(fn_cat_map.c_str());
    if(!f.is_open()){
	fprintf(stderr,"Cannot open: %s: %s",fn_cat_map.c_str(),strerror(errno));
        exit(1);
    }
    string line;
    while(getline(f,line)){
	int class_id;
	char class_name[1024];
	if(sscanf(line.c_str(),"%d %s", &class_id, class_name) == 2){
	    classNames[ class_id ] = std::string(class_name);
	}
    }
    
    // step 2 - load the LinearOvaSVM objet
    ClassifierConfig cc;
    recognizer = new LinearOvaSVM(cc,fn_modelfile);
}

/**
 * Runs with the pre-trained recognizer
 */
void lift_svm::load_pretrained()
{
    for(size_t i=0;ovas[i];i++){
	const struct LinearOvaSVM::linear_ova_config_t &ova = *ovas[i];
	classNames[ ova.class_id ] = ova.class_name;
    }
    ClassifierConfig cc;
    recognizer = new LinearOvaSVM(cc,ovas);
}


void lift_svm::save_cpp(const std::string &outfile)
{
    std::ofstream os(outfile.c_str());
    if(!os.is_open()){
	fprintf(stderr,"Cannot open: %s: %s",outfile.c_str(),strerror(errno));
        exit(1);
    }
    os << "#include \"lift_svm.h\"\n";
    os << "const struct lift_svm::classnames_t ova_classnames[] = {";
    EACH(it,classNames){
	os << "{" << it->first << ",\"" << it->second << "\"},\n";
    }
    os << "{0,0}};\n";
    for(size_t B=0;B<recognizer->Bcls.size();B++){
	os << "double ova_weights" << B << "[] = {\n";
	for(size_t i=0;i<recognizer->Bcls[B]->wt_max;i++){
	    if(i>0) os << ",";
	    if(i>0 && i%10==0) os << "\n";
	    os << recognizer->Bcls[B]->weights[i];
	}
	os << "};\n";
	os << "struct LinearOvaSVM::linear_ova_config_t ova"
	   << B << " = {"
	   << recognizer->classes[B] << ","
	   << "\"" << classNames[recognizer->classes[B]] << "\"" << ","
	   << recognizer->Bcls[B]->wt_max << ","
	   << recognizer->Bcls[B]->positive  << ","
	   << recognizer->Bcls[B]->negative  << "," 
	   << recognizer->Bcls[B]->threshold << ","
	   << "ova_weights" << B << " };\n";
    }
    os << "const struct LinearOvaSVM::linear_ova_config_t *lift_svm::ovas[] = {\n";
    for(size_t B=0;B<recognizer->Bcls.size();B++){
	os << "    &ova" << B << ",\n";
    }
    os << "    0};\n";
    os.close();
    std::cout << "done\n";
}


//---------------------------------------
//----- Classification Algorithms -------

/**
 * create_ngram:
 * Return a vector of weighted ngrams
 */
feature lift_svm::create_ngram(const sbuf_t &sbuf) const
{
    assert( ngram > 0 && ngram < 4 );	// we can do 1, 2 or 3 grams

    map<int, double> counts;		// map of [ngram_id] -> frequency
    feature ret;

    for(size_t i = 0; i < sbuf.bufsize-ngram+1; i++){
	uint32_t lid = 0;
	switch(ngram){
	case 1:
	    lid = sbuf[i];
	    break;
	case 2:
	    lid = (sbuf[i]<<8)  | (sbuf[i+1]);
	    break;
	case 3:
	    lid = (sbuf[i]<<16) | (sbuf[i+1]<<8) | sbuf[i+2];
	    break;
	}
	counts[lid+1]++; // not clear why we add 1, but CMU did; is counts[0] special?
    }

    /* Apply cosine normalization */
    double sum_of_squares = 0;
    
    EACH(it,counts) sum_of_squares += (it->second) * (it->second); 
    double rss = sqrt( sum_of_squares );
    ret = feature ( counts.begin() , counts.end() );
    if( rss > (1e-9) ) EACH(it,ret) it->second /= rss; // normalize if dividing by RSS won't cause divizion by zero
    return ret;
}


/**
 * lift_svm(sbuf) takes a sbuf, extracts ngrams, classifies it with
 * all classifiers, returns the one with the highest score.
 *
 * @param sbuf    - sbuf to be processed
 * @return string - classification result ("" if no result)
 */
string lift_svm::classify(const sbuf_t &sbuf,double *myScore ) const
{
    Document doc;

    doc.Features   = create_ngram(sbuf);
    
    feature scores = recognizer->get_classifier_score( doc );

    bool first     = true;
    double max_value = 0;
    int max_index = 0;
 
    for(feature::const_iterator it=scores.begin();it!=scores.end();it++){
	if( first || (it->second > max_value) ) {
	    max_value = it->second;
	    max_index = it->first;
	    first = false;
	}
    }

    assert( classNames.count( max_index ) );	// verify we have a point with a count
    if(myScore) *myScore = max_value;
    return classNames.at(max_index); 
} 

void lift_svm::dump(std::ostream &os) const
{
    std::cout << "classifier catmap:\n";
    for(lift_svm::classNames_map_t::const_iterator it = classNames.begin(); it!=classNames.end(); it++){
	std::cout << "classNames[" << it->first << "] = " << it->second << "\n";
    }
    if(recognizer){
	recognizer->dump(os);
    }
}
