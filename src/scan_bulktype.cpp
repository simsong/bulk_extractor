#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "dig.h"
#include "be13_api/utils.h"
#include <math.h>

#include <algorithm>

static uint32_t bulktype_mode = 1;
static size_t   bulk_block_size = 4096;	// default block size

/**
 * scan_bulk implements the bulk data analysis system.
 * The data analysis system was originally written to solve the DFRWS 2012 challenge.
 * http://www.dfrws.org/2012/challenge/.  It was rewritten to use the modified tag system
 * to perform disk storage inventory.
 * 
 * Method of operation:
 *
 * 1. Various scanners will perform bulk data analysis and write to a tag file.
 *
 * 2. If scan_bulk is called recursively, it will note the recursive
 * calls in the tag file as well. This provides tagging for recursive
 * scanners like BASE64 which do not write to tags.
 * 
 * 3. On post-processing, scan_bulk will look through all of the other feature files
 * and write to stdout a sorted file with all offsets and identified contents.
 *
 * There exist a number of hard-coded rules to improve the output.
 * 
 * rule 1 - If constant data is detected, no comments are provided.
 * rule 2 - If there is EXIF data, it's a JPEG
 * 
 * Scan_bulk can call additional bulk data analysis scanners.
 * They all use the standard bulk_extractor plug-in API.
 */

static const std::string CONSTANT("constant");
static const std::string JPEG("jpeg");
static const std::string RANDOM("random");
static const std::string HUFFMAN("huffman_compressed");

static int debug=0;


float  opt_high_entropy         = 7.0;	// the level at which we alert
bool   opt_low_entropy		= false;  // true if we report low entropy
float  opt_MinimumCosineVariance = 0.999; // above this is random; below is huffman
int    opt_VectorLen = 250;
size_t sd_acv_min_buf = 4096;

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
typedef std::vector<sector_typetag> sector_typetags_vector_t; //
sector_typetags_vector_t sector_typetags;		      // what gets put where


static void bulk_process_feature_file(const std::string &fn)
{
    if(ends_with(fn,".txt")==false) return; // don't process binary files
    if(ends_with(fn,"_histogram.txt")==true) return; // ignore histogram files

    const std::string features(fn.substr(fn.rfind('/')+1,fn.size()-fn.rfind('/')-5));
    const std::string name(features+": ");
    const std::string SPACE(" ");
    const std::string UNKNOWN("UNKNOWN");
    bool tagfile = ends_with(fn,"_tags.txt");
    std::ifstream f(fn.c_str());
    if(!f.is_open()){
        std::cerr << "Cannot open tag input file: " << fn << "\n";
	return;
    }
    try {
	std::string line;
	while(getline(f,line)){
	    if(line.size()==0 || line[0]=='#' || line.substr(0,4)=="\357\273\277#") continue;
            std::vector<std::string> fields = split(line,'\t'); // fields of the feature file
	    if(fields.size()<2) continue;	      // improper formatting
	    std::string &taglocation  = fields[0];
	    std::string &tagtype = fields[1];
	    uint64_t offset = stoi64(taglocation);
	    uint64_t sector =  offset / bulk_block_size;

	    /* If the array hasn't been expanded to the point of this element, expand it with blanks */
	    while(sector > sector_typetags.size()){
		sector_typetags.push_back(sector_typetag()); // expand to fill gap
	    }

	    if(tagfile){		// first pass
		/* Process a tag */
                std::vector<std::string> vals  = split(taglocation,':');
		
		if(vals.size()!=2){
		    std::cerr << "Invalid tag file line: " << line << " (size=" << vals.size() << ")\n";
		    exit(1);
		}

		uint32_t len = stoi(vals[1]);

		// If no data for this sector, simply append this type
		// and then continue
		if(sector_typetags.size()==sector){ 
		    sector_typetags.push_back(sector_typetag(len,tagtype,std::string("")));
		    continue;
		} 

		// We have new data for the same element. Which is better?
		if(sector_typetags[sector].specificity() < sector_typetag::specificity(tagtype)){
		    // New is more specific than the old.
		    // Preserve the old one 
		    sector_typetags[sector].scomment = sector_typetags[sector].stype + std::string("; ") + sector_typetags[sector].scomment;
		    sector_typetags[sector].stype = tagtype; // specify new tag type
		} else {
		    // New is less specific than the old, so just make new type a comment.
		    sector_typetags[sector].scomment = tagtype + std::string("; ") + sector_typetags[sector].scomment;
		}
		continue;
	    }
	    /* Process a feature, which will add specificity to the tag */
	    if(sector_typetags.size()==sector){ 
		/* Hm... No tag (and all tags got processed first), so this is unknown */
		sector_typetags.push_back(sector_typetag(bulk_block_size,UNKNOWN,SPACE));
	    } 
	    /* append what we've learned regarding this feature */
	    
	    // If we got an MD5 as field1 and there is a second field, go with that
	    
	    sector_typetag &s = sector_typetags[sector];
	    int field = 1;
	    if(fields.size()>2 && fields[1].size()==32 && fields[2].size()>0 && fields[2][0]=='<'){
		field = 2;		// go with the second field
	    }
	    s.scomment += " " + name + fields[field];
	    
	    // append any XML if it is present
	    if(field==1 && fields.size()>2 && fields[2].size()>0 && fields[2][0]=='<'){
		s.scomment += " " + name + " " + fields[2];
	    }

	    // Scan through the feature indicator table and if we find a match note the type
	    for(int i=0 ; feature_indicators[i].feature_file_name;i++){
		if(features!=feature_indicators[i].feature_file_name) continue;
		if(feature_indicators[i].feature_content==0
		   || fields[1].find(feature_indicators[i].feature_content)!=std::string::npos
		   || fields[2].find(feature_indicators[i].feature_content)!=std::string::npos){
		    s.stype = pos0_t(fields[0]).alphaPart();
		    if(s.stype.size()>1){
			char lastchar = s.stype.at(s.stype.size()-1);
			if(lastchar!='-' && lastchar!='/') s.stype += std::string("-");
		    }
		    s.stype += feature_indicators[i].dfrws_type;
		}
	    }
	}
    }
    catch (const std::exception &e) {
	cerr << "ERROR: " << e.what() << " processing tagfile " << fn << "\n";
    }
}

#ifdef WIN32
static void bulk_process_feature_file(const dig::filename_t &fn16)
{
    std::string fn8;
    utf8::utf16to8(fn16.begin(),fn16.end(),back_inserter(fn8));
    bulk_process_feature_file(fn8);
}
#endif


/**
 * Internal histogram class, can also handle entropy calculations
 */
class bulk_histogram {
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
	for(vector<hist_element *>::const_iterator it = counts.begin();it!=counts.end();it++){
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
	for(vector<hist_element *>::const_iterator it = counts.begin();it!=counts.end();it++){
	    float p = (float)(*it)->count / (float)bulk_block_size;
	    eval += -p * log2(p);
	}
	return eval;
    }
    int unique_counts() {
	return counts.size();
    };
};
vector<float> bulk_histogram::entropy_array;	// where things get store


/* 
 * This is currently a quick-and-dirty tester for the random vs. huffman characterizer.
 * The goal is to turn it into the general purpose production framework.
 * Returns true if 
 */

class scan_bulktype {
public:;
    scan_bulktype(const sbuf_t &sbuf_,feature_recorder_set &fs_):sbuf(sbuf_),fs(fs_){
    };
    const sbuf_t &sbuf;
    feature_recroder_set &fs;
    double sd_autocorrelation_cosine_variance(const bulk_histogram &sbufhist);
    void bulk_ngram_entropy();
    bool bulk_bitlocker();
};


double scan_bulktype::sd_autocorrelation_cosine_variance(const bulk_histogram &sbufhist)
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
	bulk_histogram autohist;
	autohist.add(autobuf.buf,sbuf.bufsize);
	autohist.calc_distribution();

	/* Now compute the cosine similarity */
	double dotproduct = 0.0;
	for(size_t i=0; i < sbufhist.counts.size()  && i < autohist.counts.size() && i<256;
	    i++){
	    dotproduct += sbufhist.counts[i]->count * autohist.counts[i]->count;
	}

	double mag_squared1 = 0.0;
	double mag_squared2 = 0.0;
	for(size_t i=0;i<256;i++){
	    if(i < sbufhist.counts.size()){
		mag_squared1 += sbufhist.counts[i]->count * sbufhist.counts[i]->count;
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

void scan_bulktype::bulk_ngram_entropy()
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
	    bulk_tags->write_tag(sbuf,ss.str());
	    return;			// ngram is better than entropy
	}
    }

    /* Couldn't find ngram; check entropy and FF00 counts...*/
    size_t ff00_count=0;
    bulk_histogram h;
    h.add(sbuf.buf,sbuf.pagesize);
    h.calc_distribution();		

    for(size_t i=0;i<sbuf.pagesize-1;i++){
	if(sbuf[i]==0xff && sbuf[i+1]==0x00) ff00_count++;
    }

    if(sbuf.pagesize<=4096){		// we only tuned for 4096
	if(ff00_count>2 && h.unique_counts()>=220){
	    bulk_tags->write_tag(sbuf,JPEG);
	    return;
	}
    }

    float entropy = h.entropy();

    std::stringstream ss;
    if(entropy>opt_high_entropy){
	float cosineVariance = sd_autocorrelation_cosine_variance(h);
	if(debug & DEBUG_INFO) ss << "high entropy ( S=" << entropy << ")" << " ACV= " << cosineVariance << " ";
	if(cosineVariance > opt_MinimumCosineVariance){
	    ss << RANDOM;
	} else {
	    ss << HUFFMAN;
	}
	bulk_tags->write_tag(sbuf,ss.str());
    } 
    // don't bother recording 'low entropy' unless debuggin
    if(entropy<=opt_high_entropy && opt_low_entropy) {
	ss << "low entropy ( S=" << entropy << ")";
	if(h.unique_counts() < 5){
	    ss << " Unique Counts: " << h.unique_counts() << " ";
	    for(vector<bulk_histogram::hist_element *>::const_iterator it = h.counts.begin();it!=h.counts.end();it++){
		ss << (int)((*it)->val) << ":" << (*it)->count << " ";
	    }
	}
    }
}
    

bool scan_bulktype::bulk_bitlocker()
{
    /* Look for a bitlocker start */
    /* Start of bitlocker protected volume */
    static uint8_t BitLockerStart[] = {0xEB,0x52,0x90,0x2D,0x46,0x56,0x45,0x2D,0x46,0x53,0x2D};
    return sbuf.bufsize >= sizeof(BitLockerStart)
        && memcmp(sbuf.buf,BitLockerStart,sizeof(BitLockerStart))==0;
}

void scanner_bulktype::process()
{
    bulk_ngram_entropy(sbuf,bulk,bulk_tags);
    bulk_bitlocker(sbuf,bulk,bulk_tags);
}

#ifndef _TEXT 
#define _TEXT(x) x
#endif

extern "C"
void scan_bulk(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      

    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "bulk";
	sp.info->author		= "Simson Garfinkel";
	sp.info->description	= "perform bulk data type scan";
	sp.info->flags		= (scanner_info::SCANNER_DISABLED
                                   | scanner_info::SCANNER_WANTS_NGRAMS
                                   | scanner_info::SCANNER_NO_ALL);
        sp.info->get_config("bulktype_mode",&bulktype_mode,"1=histogram only; 2=all tags");
        sp.info->get_config("bulk_block_size",&bulk_block_size,"Block size (in bytes) for bulk data analysis");
        sp.info->get_config("bulktype_debug",&debug,"bulk debug mode");

	bulk_histogram::precalc_entropy_array(bulk_block_size);

        if(bulktype_mode==2){
            sp.info->feature_names.insert("bulk_tags");
        }

        return; 
    }

    // classify a buffer
    if(sp.phase==scanner_params::PHASE_SCAN){

	feature_recorder *bulk_tags = sp.fs.get_name("bulk_tags");
    
	if(sp.sbuf.pos0.isRecursive()){
	    /* Record the fact that a recursive call was made, which tells us about the data */
	    bulk_tags->write_tag(sp.sbuf,""); // tag that we found recursive data, not sure what kind
	}

	if(sp.sbuf.pagesize < bulk_block_size) return; // can't analyze something this small

	// Loop through the sbuf in bulk_block_size sized chunks
	// for each one, examine the entropy and scan for bitlocker (unfortunately hardcoded)
	// This needs to have a general plug-in architecture
	for(size_t base=0;base+bulk_block_size<=sp.sbuf.pagesize;base+=bulk_block_size){
	    sbuf_t sbuf(sp.sbuf,base,bulk_block_size);
            scan_bulktype scanner(sbuf,sp.fs);
            scanner.process();
        }
    }
}


