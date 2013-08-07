#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint32_t word_min = 6;
static uint32_t word_max = 14;
static uint64_t max_word_outfile_size=100*1000*1000;

class WordlistSorter {
public:
    bool operator()(const string &a,const string &b) {
	if(a.size() < b.size()) return true;
	if(a.size() > b.size()) return false;
	return a<b;
    }
};

static void wordlist_split_and_dedup(string outdir_)
{
    cout << "Phase 3. Uniquifying and recombining wordlist\n";

    string ifn = outdir_+"/wordlist.txt";
    string ofn_template = outdir_+"/wordlist_split_%03d.txt";
    ifstream f2(ifn.c_str());
    if(!f2.is_open()) err(1,"Cannot open %s\n",ifn.c_str());
    ofstream of2;
    int of2_counter = 0;
    uint64_t outfilesize = 0;

    /* Read all of the words */

    while(!f2.eof()){
	// set is the sorted list of words we have seen
	set<string,WordlistSorter> seen;	
	while(!f2.eof()){
	    /* Create the first file (of2==0) or roll-over if outfilesize>100M */
	    string line;
	    getline(f2,line);
	    if(line[0]=='#') continue;	// ignore comments
	    size_t t1 = line.find('\t');		// find the beginning of the feature
	    if(t1!=string::npos) line = line.substr(t1+1);
	    size_t t2 = line.find('\t');		// find the end of the feature
	    if(t2!=string::npos) line = line.substr(0,t2);
	    try {
		if(line.size()>0) seen.insert(line);
	    }
	    catch (std::bad_alloc &er) {
		std::cerr << er.what() << std::endl;
		std::cerr << "Dumping current dataset; will then restart dedup." << std::endl;
		break;
	    }
	}
	/* Dump the words so far */

	for(set<string,WordlistSorter>::const_iterator it = seen.begin();it!=seen.end();it++){
	    if(!of2.is_open() || outfilesize>max_word_outfile_size){ 
		if(of2.is_open()) of2.close();
		char fname[128];
		snprintf(fname,sizeof(fname),ofn_template.c_str(),of2_counter++);
		of2.open(fname);
		if(!of2.is_open()) err(1,"Cannot open %s",fname);
		outfilesize = 0;
	    }
	    
	    if ((*it).size()>0) {
		of2 << (*it) << '\n';
		outfilesize += (*it).size() + 1;
	    }
	}
    }
    if(of2.is_open()) of2.close();
    f2.close();
}

static bool wordchar[256];
inline bool wordchar_func(unsigned char ch)
{
    return isprint(ch) && ch!=' ' && ch<128;
}

static void wordchar_setup()
{
    for(int i=0;i<256;i++){
	wordchar[i] = wordchar_func(i);
    }
}

extern "C" 
void scan_wordlist(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "wordlist";
        sp.info->flags = scanner_info::SCANNER_DISABLED;
        sp.info->feature_names.insert("wordlist");
        sp.info->get_config("word_min",&word_min,"Minimum word size");
        sp.info->get_config("word_max",&word_max,"Maximum word size");
        sp.info->get_config("max_word_outfile_size",&max_word_outfile_size,"Maximum size of the words output file");
        if(word_min>word_max){
            fprintf(stderr,"ERROR: word_min=%d word_max=%d\n",word_min,word_max);
            exit(1);
        }
	wordchar_setup();
	return;
    }
    feature_recorder_set &fs = sp.fs;
    feature_recorder *wordlist_recorder = fs.get_name("wordlist");
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
	wordlist_split_and_dedup(sp.fs.outdir);
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = sp.sbuf;

	/* Look for words in the buffer. Runs a finite state machine.
	 * for each character in the buffer:
	 * case 1 - we are not in a word & this character starts a word.
	 * case 2 - we are in a word & this character ends a word.
	 */
    
	wordlist_recorder->set_flag(feature_recorder::FLAG_NO_CONTEXT);  // not useful for wordlist
	wordlist_recorder->set_flag(feature_recorder::FLAG_NO_STOPLIST); // not necessary for wordlist
	wordlist_recorder->set_flag(feature_recorder::FLAG_NO_ALERTLIST); // not necessary for wordlist

	int wordstart = -1;			// >=0 means we are in a word
	for(u_int i=0;i<sbuf.pagesize;i++){
	    /* case 1 */
	    bool iswordchar = wordchar[sbuf.buf[i]];
	    if(wordstart<0 && iswordchar && i!=sbuf.pagesize-1){
		wordstart = i;
		continue;
	    }
	    if(wordstart>=0 && (!iswordchar || i==sbuf.pagesize-1)){
		uint32_t len = i-wordstart;
		if((word_min <= len) && (len <=  word_max)){
		    wordlist_recorder->write_buf(sbuf,wordstart,len);
		}
		wordstart = -1;
	    }
	}
    }
}
