#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "dig.h"
#include "be13_api/utils.h"
#include "histogram.h"
#include <math.h>
#include <algorithm>

#include "sceadan/src/sceadan.h"

/**
 * scan_sceadan implements the bulk data analysis system.
 * 
 * Method of operation:
 *
 * 1. Various scanners will perform bulk data analysis and write to a tag file.
 *
 * 2. If scan_sceadan is called recursively, it will note the recursive
 * calls in the tag file as well. This provides tagging for recursive
 * scanners like BASE64 which do not write to tags.
 * 
 * 3. On post-processing, scan_sceadan will look through all of the other feature files
 * and write to stdout a sorted file with all offsets and identified contents.
 *
 * There exist a number of hard-coded rules to improve the output.
 * 
 * rule 1 - If constant data is detected, no comments are provided.
 * rule 2 - If there is EXIF data, it's a JPEG
 * 
 * scan_sceadan can call additional bulk data analysis scanners.
 * They all use the standard bulk_extractor plug-in API.
 */

static const std::string CONSTANT("constant");
static const std::string JPEG("jpeg");
static const std::string RANDOM("random");
static const std::string HUFFMAN("huffman_compressed");

static int debug=0;

#if 0
/* Voting on the contents of each sector */
class sector_typetag {
public:
    /* the specificity is how 'specific' a type identification is.
     * It's currently 10 points for each slash and 5 point for each character
     * that is directly after a slash.
     * A 0-length is never specific.
     */
    static int specificity(const std::string &str){
	if(str.size()==0) return -100;
	int count = 0;
	for(size_t i=0;i<str.size();i++){
	    if(str[i]=='/') count += 10;
	    if(str[i]=='/' && i+1<str.size()) count += 5;
	}
	return count;
    }
    sector_typetag(uint32_t bytes_,const std::string &stype_,const std::string &scomment_):
	bytes(bytes_),stype(stype_),scomment(scomment_){}
    sector_typetag():bytes(0),stype(),scomment(){}
    uint32_t bytes;			// number of bytes in this tag
    std::string stype;			// the sector type
    std::string scomment;		// the comments
    int specificity(){			// number of slashes in stype; a measure of the specificity
	return specificity(stype);
    }
};
std::ostream & operator<< (std::ostream &os, const sector_typetag &ss) {
    os << "[" << ss.bytes << " " << ss.stype << " " << ss.scomment << "]";
    return os;
};
typedef vector<sector_typetag> sector_typetags_vector_t; //
sector_typetags_vector_t sector_typetags;		      // what gets put where
#endif

static size_t opt_bulk_block_size = 512;	// 


/* Start of bitlocker protected volume */
float  opt_high_entropy         = 7.0;	// the level at which we alert
bool   opt_low_entropy		= false;  // true if we report low entropy
float  opt_MinimumCosineVariance = 0.999; // above this is random; below is huffman
int    opt_VectorLen = 250;
size_t sd_acv_min_buf = 4096;


/**
 * Internal histogram class, can also handle entropy calculations
 */
class histogram {
public:;
    class hist_element {
    public:;
	hist_element(uint8_t v,uint32_t c):val(v),count(c){};
	hist_element(const hist_element &ce):val(ce.val),count(ce.count){};
	uint8_t  val;			// the value
	uint32_t count;			// how many we have seen
	static int inverse_frequency(const hist_element *a,const hist_element *b){
	    return a->count > b->count;
	}
    };
    
    histogram():counts(){}
    virtual ~histogram(){
	for(std::vector<hist_element *>::const_iterator it = counts.begin();it!=counts.end();it++){
	    delete *it;
	}
	counts.clear();
    }
	
    /* Entropy array is a precomputed array of p*log(p) where p=counts/blocksize
     * index is number of counts.
     */
    static std::vector<float> entropy_array;
    std::vector<hist_element *> counts;	// histogram counts, sorted by most popular
    static void precalc_entropy_array(int blocksize) {
	entropy_array.clear();
	for(int i=0;i<blocksize+1;i++){
	    double p = (double)i/(double)blocksize;
	    entropy_array.push_back(-p * log2(p));
	}
    }
    uint32_t hist[256];			// counts of each byte value
    void add(const uint8_t *buf,size_t len){
	memset(hist,0,sizeof(hist));
	for(size_t i=0;i<len;i++){
	    hist[buf[i]]++;
	}
    }
    /**
     * Calculate the number of unique values and sort them.
     */
    void calc_distribution(){	
	for(int i=0;i<256;i++){
	    if(hist[i]) counts.push_back(new hist_element(i,hist[i]));
	}
	/* And sort it by frequency */
	sort(counts.begin(),counts.end(),hist_element::inverse_frequency);
    }
    float entropy() {
	float eval = 0;
	for(std::vector<hist_element *>::const_iterator it = counts.begin();it!=counts.end();it++){
	    float p = (float)(*it)->count / (float)opt_bulk_block_size;
	    eval += -p * log2(p);
	}
	return eval;
    }
    int unique_counts() {
	return counts.size();
    };
};
std::vector<float> histogram::entropy_array;	// where things get store


/**
 * classify an sbuf and report the results.
 */
class sector_classifier {
    // default copy construction and assignment are meaningless
    // and not implemented
    sector_classifier(const sector_classifier &);
    sector_classifier &operator=(const sector_classifier &);

    static const uint8_t BitLockerStart[];
    feature_recorder *bulk_fr;
    histogram h;
    const sbuf_t &sbuf;
    void check_ngram_entropy();
    bool check_bitlocker();
public:;
    sector_classifier(feature_recorder *b,const sbuf_t &sbuf_):bulk_fr(b),h(),sbuf(sbuf_){}
    double sd_autocorrelation_cosine_variance() const;
    void run() {
        check_ngram_entropy();
        check_bitlocker();
    }
};
const uint8_t sector_classifier::BitLockerStart[] = {0xEB,0x52,0x90,0x2D,0x46,0x56,0x45,0x2D,0x46,0x53,0x2D};

/* 
 * This is currently a quick-and-dirty tester for the random vs. huffman characterizer.
 * The goal is to turn it into the general purpose production framework.
 */

double sector_classifier::sd_autocorrelation_cosine_variance() const
{
    if(sbuf.bufsize<sd_acv_min_buf) return 0;
    if(sbuf.bufsize==0) return 0;

    /* Create the autocorolation buffer */
    managed_malloc<uint8_t>autobuf(sbuf.bufsize);
    if(autobuf.buf){
	for(size_t i=0;i<sbuf.bufsize-1;i++){
	    autobuf.buf[i] = sbuf[i] + sbuf[i+1];
	}
	autobuf.buf[sbuf.bufsize-1] = sbuf[sbuf.bufsize-1] + sbuf[0];

	/* Get a histogram for autobuf */
	histogram autohist;
	autohist.add(autobuf.buf,sbuf.bufsize);
	autohist.calc_distribution();

	/* Now compute the cosine similarity */
	double dotproduct = 0.0;
	for(size_t i=0; i < h.counts.size()  && i < autohist.counts.size() && i<256;
	    i++){
	    dotproduct += h.counts[i]->count * autohist.counts[i]->count;
	}

	double mag_squared1 = 0.0;
	double mag_squared2 = 0.0;
	for(size_t i=0;i<256;i++){
	    if(i < h.counts.size()){
		mag_squared1 += h.counts[i]->count * h.counts[i]->count;
	    }
	    if(i < autohist.counts.size()){
		mag_squared2 += autohist.counts[i]->count * autohist.counts[i]->count;
	    }
	}
	double mag1 = sqrt(mag_squared1);
	double mag2 = sqrt(mag_squared2);
	double cosinesim = dotproduct / (mag1 * mag2);
	return cosinesim;
    }
    else return 0;
}

/**
 * perform a variety of tests based on ngrams and entropy.
 * This is largely a result of our paper,
 * "Using purpose-built functions and block hashes to enable small block and sub-file forensics,"
 * http://simson.net/clips/academic/2010.DFRWS.SmallBlockForensics.pdf
 */

void sector_classifier::check_ngram_entropy()
{
    /* Quickly compute the histogram */ 

    /* Scan for a repeating ngram */
    for(size_t ngram_size = 1; ngram_size < 20; ngram_size++){
	bool ngram_match = true;
	for(size_t i=ngram_size;i<sbuf.pagesize && ngram_match;i++){
	    if(sbuf[i%ngram_size]!=sbuf[i]) ngram_match = false;
	}
	if(ngram_match){
            std::stringstream ss;
	    ss << CONSTANT << "(";
	    for(size_t i=0;i<ngram_size;i++){
		char buf[16];
		snprintf(buf,sizeof(buf),"0x%02x",sbuf[i]);
		if(i>0) ss << ' ';
		ss << buf;
	    }
	    ss << ")";
	    bulk_fr->write(sbuf.pos0,ss.str(),"");
	    return;			// ngram is better than entropy
	}
    }


    /* Couldn't find ngram; check entropy and FF00 counts...*/
    size_t ff00_count=0;
    h.add(sbuf.buf,sbuf.pagesize);
    h.calc_distribution();		

    for(size_t i=0;i<sbuf.pagesize-1;i++){
	if(sbuf[i]==0xff && sbuf[i+1]==0x00) ff00_count++;
    }

    if(sbuf.pagesize<=4096){		// we only tuned for 4096
	if(ff00_count>2 && h.unique_counts()>=220){
	    bulk_fr->write(sbuf.pos0,JPEG,"");
	    return;
	}
    }

    float entropy = h.entropy();

    std::stringstream ss;
    if(entropy>opt_high_entropy){
	float cosineVariance = sd_autocorrelation_cosine_variance();
	if(debug & DEBUG_INFO) ss << "high entropy ( S=" << entropy << ")" << " ACV= " << cosineVariance << " ";
	if(cosineVariance > opt_MinimumCosineVariance){
	    ss << RANDOM;
	} else {
	    ss << HUFFMAN;
	}
	bulk_fr->write(sbuf.pos0,ss.str(),"");
    } 

    // don't bother recording 'low entropy' unless debuggin
    if(entropy<=opt_high_entropy && opt_low_entropy) {
	ss << "low entropy ( S=" << entropy << ")";
	if(h.unique_counts() < 5){
	    ss << " Unique Counts: " << h.unique_counts() << " ";
	    for(std::vector<histogram::hist_element *>::const_iterator it = h.counts.begin();it!=h.counts.end();it++){
		ss << (int)((*it)->val) << ":" << (*it)->count << " ";
	    }
	}
    }
}
    

bool sector_classifier::check_bitlocker()
{
    /* Look for a bitlocker start */
    if(sbuf.bufsize >= sizeof(BitLockerStart)
       && memcmp(sbuf.buf,BitLockerStart,sizeof(BitLockerStart))==0){
	bulk_fr->write(sbuf.pos0,"BITLOCKER HEADER","");
        return true;
    }
    return false;
}


extern "C"
void scan_sceadan(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      

    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "bulk";
	sp.info->author		= "Simson Garfinkel";
	sp.info->description	= "perform bulk data scan";
	sp.info->flags		= (scanner_info::SCANNER_DISABLED
                                   | scanner_info::SCANNER_WANTS_NGRAMS | scanner_info::SCANNER_NO_ALL);
	sp.info->feature_names.insert("bulk");
	sp.info->feature_names.insert("bulk");
        sp.info->get_config("bulk_block_size",&opt_bulk_block_size,"Block size (in bytes) for bulk data analysis");

        debug = sp.info->config->debug;
	histogram::precalc_entropy_array(opt_bulk_block_size);
        return; 
    }

    feature_recorder *bulk_fr = sp.fs.get_name("bulk");
    if(!bulk_fr) return;                // all of the remianing steps need bulk_fr
    if(sp.phase==scanner_params::PHASE_INIT){
        bulk_fr->set_flag(feature_recorder::FLAG_NO_CONTEXT |
                          feature_recorder::FLAG_NO_STOPLIST |
                          feature_recorder::FLAG_NO_ALERTLIST |
                          feature_recorder::FLAG_NO_FEATURES );
    }

    // classify a buffer
    if(sp.phase==scanner_params::PHASE_SCAN){

	//if(sp.sbuf.pos0.isRecursive()){
	    /* Record the fact that a recursive call was made, which tells us about the data */
	//    bulk_fr->write(sp.sbuf.pos0,"",""); // tag that we found recursive data, not sure what kind
	//}

	if(sp.sbuf.pagesize < opt_bulk_block_size) return; // can't analyze something that small

	// Loop through the sbuf in opt_bulk_block_size sized chunks
	// for each one, examine the entropy and scan for bitlocker (unfortunately hardcoded)
	// This needs to have a general plug-in architecture

	for(size_t base=0;base+opt_bulk_block_size<=sp.sbuf.pagesize;base+=opt_bulk_block_size){
	    sbuf_t sbuf(sp.sbuf,base,opt_bulk_block_size);
            sector_classifier sc(bulk_fr,sbuf);
            sc.run();
	}
    }

    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
    }
}


