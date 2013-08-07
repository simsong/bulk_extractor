#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "dig.h"
#include "be13_api/utils.h"
#include <math.h>

#include <algorithm>

/**
 * scan_bulk implements the bulk data analysis system.
 * The data analysis system was largely written to solve the DFRWS 2012 challenge.
 * http://www.dfrws.org/2012/challenge/
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

/* Substitution table */
static struct replace_t {
    const char *from;
    const char *to;
} dfrws2012_replacements[] =  {
    {"constant(0x00)","null"},
    {"huffman_compressed","zlib"},
    {"gzip","zlib"},
    {0,0}
};

/* Feature indicator table */
static struct feature_indicators_t {
    const char *feature_file_name;
    const char *feature_content;
    const char *dfrws_type;
}  feature_indicators[] = {
    {"aes_keys",0,"aeskey"},
    {"elf","","elf_executable"},
    {"exif","<exif>","jpg"},
    {"json",0,"json"},
    {"kml",0,"xml-kml"},
    {"url",0,"html"},
    {"vcard",0,"txt-vcard"},
    {"windirs","fileobject src='mft'","fs-ntfs"},
    {"windirs","fileobject src='fat'","fs-fat"},
    {"winpe",0,"ms-win-prefetch"},
    {0,0,0},
};


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

static size_t opt_bulk_block_size = 512;	// 

static void bulk_process_feature_file(const std::string &fn)
{
    if(ends_with(fn,".txt")==false) return; // don't process binary files
    if(ends_with(fn,"_histogram.txt")==true) return; // ignore histogram files

    const string features(fn.substr(fn.rfind('/')+1,fn.size()-fn.rfind('/')-5));
    const string name(features+": ");
    const string SPACE(" ");
    const string UNKNOWN("UNKNOWN");
    bool tagfile = ends_with(fn,"_tags.txt");
    ifstream f(fn.c_str());
    if(!f.is_open()){
	cerr << "Cannot open tag input file: " << fn << "\n";
	return;
    }
    try {
	string line;
	while(getline(f,line)){
	    if(line.size()==0 || line[0]=='#' || line.substr(0,4)=="\357\273\277#") continue;
	    vector<string> fields = split(line,'\t'); // fields of the feature file
	    if(fields.size()<2) continue;	      // improper formatting
	    std::string &taglocation  = fields[0];
	    std::string &tagtype = fields[1];
	    uint64_t offset = stoi64(taglocation);
	    uint64_t sector =  offset / opt_bulk_block_size;

	    /* If the array hasn't been expanded to the point of this element, expand it with blanks */
	    while(sector > sector_typetags.size()){
		sector_typetags.push_back(sector_typetag()); // expand to fill gap
	    }

	    if(tagfile){		// first pass
		/* Process a tag */
		vector<string> vals  = split(taglocation,':');
		
		if(vals.size()!=2){
		    std::cerr << "Invalid tag file line: " << line << " (size=" << vals.size() << ")\n";
		    exit(1);
		}

		uint32_t len = stoi(vals[1]);

		// If no data for this sector, simply append this type
		// and then continue
		if(sector_typetags.size()==sector){ 
		    sector_typetags.push_back(sector_typetag(len,tagtype,string("")));
		    continue;
		} 

		// We have new data for the same element. Which is better?
		if(sector_typetags[sector].specificity() < sector_typetag::specificity(tagtype)){
		    // New is more specific than the old.
		    // Preserve the old one 
		    sector_typetags[sector].scomment = sector_typetags[sector].stype + string("; ") + sector_typetags[sector].scomment;
		    sector_typetags[sector].stype = tagtype; // specify new tag type
		} else {
		    // New is less specific than the old, so just make new type a comment.
		    sector_typetags[sector].scomment = tagtype + string("; ") + sector_typetags[sector].scomment;
		}
		continue;
	    }
	    /* Process a feature, which will add specificity to the tag */
	    if(sector_typetags.size()==sector){ 
		/* Hm... No tag (and all tags got processed first), so this is unknown */
		sector_typetags.push_back(sector_typetag(opt_bulk_block_size,UNKNOWN,SPACE));
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
	    for(int i=0;feature_indicators[i].feature_file_name;i++){
		if(features!=feature_indicators[i].feature_file_name) continue;
		if(feature_indicators[i].feature_content==0
		   || fields[1].find(feature_indicators[i].feature_content)!=string::npos
		   || fields[2].find(feature_indicators[i].feature_content)!=string::npos){
		    s.stype = pos0_t(fields[0]).alphaPart();
		    if(s.stype.size()>1){
			char lastchar = s.stype.at(s.stype.size()-1);
			if(lastchar!='-' && lastchar!='/') s.stype += string("-");
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

static void dfrws2012_bulk_process_dump()
{
    for(size_t i=0;i<sector_typetags.size();i++){
	sector_typetag &s = sector_typetags[i];
	uint64_t    offset = i * opt_bulk_block_size;
	std::string ctype = (s.stype.size()>0 ? s.stype : "UNRECOGNIZED CONTENT");
	bool print_comment = s.scomment.size() > 0;

	if(ctype.substr(0,CONSTANT.size())==CONSTANT) { // rule 1 - Constant has no comment
	    print_comment = false;
	}

	/* Leading slash gets removed */
	if(ctype.at(0)=='/') ctype.erase(0,1);

	/* Identifiers get moved to lower case and slashes get changed to dashes */
	for(size_t j=0;j<ctype.size();j++){
	    if(isupper(ctype[j])) ctype[j] = tolower(ctype[j]);
	    if(ctype[j]=='/') ctype[j]='-';
	}
	
	/* Remove anything inside braces */
	while(true){
	    size_t open_br = ctype.find('{');
	    if(open_br==std::string::npos) break;
	    if(open_br>0 && ctype[open_br-1]==' ') open_br--; // gobble the space too
	    size_t close_br = ctype.find('}',open_br);
	    if(close_br != std::string::npos){
		ctype.replace(open_br,close_br,"");
	    }
	}

	/* Process the replacement array */
	for(const replace_t *rep = dfrws2012_replacements;rep->from;rep++){
	    size_t loc = ctype.find(rep->from);
	    if(loc!=std::string::npos){
		ctype.replace(loc,strlen(rep->from),rep->to);
	    }
	}

	std::cout << offset << " " << ctype ;
	if(print_comment) std::cout << " # " << s.scomment;
	std::cout << "\n";
    }
}

/* Start of bitlocker protected volume */
static uint8_t BitLockerStart[] = {0xEB,0x52,0x90,0x2D,0x46,0x56,0x45,0x2D,0x46,0x53,0x2D};
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
	for(vector<hist_element *>::const_iterator it = counts.begin();it!=counts.end();it++){
	    delete *it;
	}
	counts.clear();
    }
	
    /* Entropy array is a precomputed array of p*log(p) where p=counts/blocksize
     * index is number of counts.
     */
    static vector<float> entropy_array;
    vector<hist_element *> counts;	// histogram counts, sorted by most popular
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
	    float p = (float)(*it)->count / (float)opt_bulk_block_size;
	    eval += -p * log2(p);
	}
	return eval;
    }
    int unique_counts() {
	return counts.size();
    };
};
vector<float> histogram::entropy_array;	// where things get store


/* 
 * This is currently a quick-and-dirty tester for the random vs. huffman characterizer.
 * The goal is to turn it into the general purpose production framework.
 * Returns true if 
 */

double sd_autocorrelation_cosine_variance(const sbuf_t &sbuf,const histogram &sbufhist)
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

static inline void bulk_ngram_entropy(const sbuf_t &sbuf,feature_recorder *bulk,feature_recorder *bulk_tags)
{
    /* Quickly compute the histogram */ 

    /* Scan for a repeating ngram */
    for(size_t ngram_size = 1; ngram_size < 20; ngram_size++){
	bool ngram_match = true;
	for(size_t i=ngram_size;i<sbuf.pagesize && ngram_match;i++){
	    if(sbuf[i%ngram_size]!=sbuf[i]) ngram_match = false;
	}
	if(ngram_match){
	    stringstream ss;
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
    histogram h;
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

    stringstream ss;
    if(entropy>opt_high_entropy){
	float cosineVariance = sd_autocorrelation_cosine_variance(sbuf,h);
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
	    for(vector<histogram::hist_element *>::const_iterator it = h.counts.begin();it!=h.counts.end();it++){
		ss << (int)((*it)->val) << ":" << (*it)->count << " ";
	    }
	}
    }
}
    

static inline void bulk_bitlocker(const sbuf_t &sbuf,feature_recorder *bulk,feature_recorder *bulk_tags)
{
    /* Look for a bitlocker start */
    if(sbuf.bufsize >= sizeof(BitLockerStart)
       && memcmp(sbuf.buf,BitLockerStart,sizeof(BitLockerStart))==0){
	bulk->write_tag(sbuf,"BITLOCKER HEADER");
    }
}


#ifndef _TEXT 
#define _TEXT(x) x
#endif

static bool dfrws_challenge = false;

extern "C"
void scan_bulk(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    // startup
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "bulk";
	sp.info->author		= "Simson Garfinkel";
	sp.info->description	= "perform bulk data scan";
	sp.info->flags		= scanner_info::SCANNER_DISABLED | scanner_info::SCANNER_WANTS_NGRAMS | scanner_info::SCANNER_NO_ALL;
	sp.info->feature_names.insert("bulk");
	sp.info->feature_names.insert("bulk_tags");
        sp.info->get_config("bulk_block_size",&opt_bulk_block_size,"Block size (in bytes) for bulk data analysis");

        debug = sp.info->config->debug;

	histogram::precalc_entropy_array(opt_bulk_block_size);

        sp.info->get_config("DFRWS2012",&dfrws_challenge,"True if running DFRWS2012 challenge code");
        return; 
    }
    // classify a buffer
    if(sp.phase==scanner_params::PHASE_SCAN){

	feature_recorder *bulk = sp.fs.get_name("bulk");
	feature_recorder *bulk_tags = sp.fs.get_name("bulk_tags");
    

	if(sp.sbuf.pos0.isRecursive()){
	    /* Record the fact that a recursive call was made, which tells us about the data */
	    bulk_tags->write_tag(sp.sbuf,""); // tag that we found recursive data, not sure what kind
	}

	if(sp.sbuf.pagesize < opt_bulk_block_size) return; // can't analyze something that small

	// Loop through the sbuf in opt_bulk_block_size sized chunks
	// for each one, examine the entropy and scan for bitlocker (unfortunately hardcoded)
	// This needs to have a general plug-in architecture
	for(size_t base=0;base+opt_bulk_block_size<=sp.sbuf.pagesize;base+=opt_bulk_block_size){
	    sbuf_t sbuf(sp.sbuf,base,opt_bulk_block_size);
	    bulk_ngram_entropy(sbuf,bulk,bulk_tags);
	    bulk_bitlocker(sbuf,bulk,bulk_tags);
	}
    }
    // shutdown --- combine the results if we are in DFRWS mode
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
	if(dfrws_challenge){
	    feature_recorder *bulk = sp.fs.get_name("bulk");
	    // First process the bulk_tags and lift_tags
	    bulk_process_feature_file(bulk->outdir + "/bulk_tags.txt"); 
	    bulk_process_feature_file(bulk->outdir + "/lift_tags.txt"); 

	    // Process the remaining feature files
	    dig d(bulk->outdir);
	    for(dig::const_iterator it = d.begin(); it!=d.end(); ++it){
		if(ends_with(*it,_TEXT("_tags.txt"))==false){
		    bulk_process_feature_file(*it);
		}
	    }
	    dfrws2012_bulk_process_dump();
	}
	return;		// no cleanup
    }
}


