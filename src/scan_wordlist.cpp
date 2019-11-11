#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint32_t word_min = 6;
static uint32_t word_max = 14;
static uint64_t max_word_outfile_size=100*1000*1000;
static bool strings = false;

/* Wordlist support for flat files */
class WordlistSorter {
public:
    bool operator()(const std::string &a,const std::string &b) const {
	if(a.size() < b.size()) return true;
	if(a.size() > b.size()) return false;
	return a<b;
    }
};
#define WORDLIST "wordlist"

/* wordlist support for SQL.  Note that the SQL-based wordlist is
 * faster than the file-based wordlist.
 */

static bool wordlist_use_flatfiles = true;

#if defined(HAVE_LIBSQLITE3) && defined(HAVE_SQLITE3_H)
#define USE_SQLITE3
#endif

#ifdef USE_SQLITE3 
static const char *schema_wordlist[] = {
    "CREATE TABLE wordlist (word BLOB)",
    "CREATE UNIQUE INDEX wordlist_i on wordlist(word)",
    0};
static const char *insert_statement = "INSERT OR IGNORE INTO wordlist VALUES (?);";
static feature_recorder::besql_stmt *wordlist_stmt=0;
static const char *select_statement = "SELECT DISTINCT word FROM wordlist ORDER BY length(word),word";
#endif


/* Global variables for writing out wordlist */
static std::ofstream of2;
static std::string ofn_template;
static int of2_counter = 0;
static uint64_t outfilesize = 0;

static void wordlist_write_word(const std::string &word)
{
    if(!of2.is_open() || outfilesize>max_word_outfile_size){ 
        if(of2.is_open()) of2.close();
        char fname[128];
        snprintf(fname,sizeof(fname),ofn_template.c_str(),of2_counter++);
        of2.open(fname);
        if(!of2.is_open()) err(1,"Cannot open %s",fname);
        outfilesize = 0;
    }
    of2 << word << '\n';
    outfilesize += word.size() + 1;
}
	    

static void wordlist_split_and_dedup(const std::string &ifn)
{
    std::ifstream f2(ifn.c_str());
    if(!f2.is_open()) err(1,"Cannot open %s\n",ifn.c_str());

    /* Read all of the words */

    while(!f2.eof()){
	// set is the sorted list of words we have seen
	std::set<std::string,WordlistSorter> seen;	
	while(!f2.eof()){
	    /* Create the first file (of2==0) or roll-over if outfilesize>100M */
	    std::string line;
	    getline(f2,line);
	    if(line[0]=='#') continue;	// ignore comments
	    size_t t1 = line.find('\t');		// find the beginning of the feature
	    if(t1!=std::string::npos) line = line.substr(t1+1);
	    size_t t2 = line.find('\t');		// find the end of the feature
	    if(t2!=std::string::npos) line = line.substr(0,t2);
            std::string word = feature_recorder::unquote_string(line);
	    try {
		if(word.size()>0) seen.insert(word);
	    }
	    catch (std::bad_alloc &er) {
		std::cerr << er.what() << std::endl;
		std::cerr << "Dumping current dataset; will then restart dedup." << std::endl;
		break;
	    }
	}
	/* Dump the words so far */

	for(std::set<std::string,WordlistSorter>::const_iterator it = seen.begin();it!=seen.end();it++){
	    if ((*it).size()>0) wordlist_write_word(*it);
        }
    }
    if(of2.is_open()) of2.close();
    f2.close();
}

/* Similar to above; write out the wordlist using SQL.
 * Not-multi-threaded, but threadsafe nonetheless.
 */
static void wordlist_sql_write(BEAPI_SQLITE3 *db3)
{
#ifdef USE_SQLITE3
    feature_recorder::besql_stmt s(db3,select_statement);
    while (sqlite3_step(s.stmt) != SQLITE_DONE) {
        const char *base = (const char *)sqlite3_column_blob(s.stmt,0);
        int len          = sqlite3_column_bytes(s.stmt,0);
        wordlist_write_word(std::string(base,len));
    }
    if(of2.is_open()) of2.close();
#endif
}


static bool wordchar[256];
inline bool wordchar_func(unsigned char ch)
{
    if(strings) {
      return isprint(ch) && ch<128;
    } else {
      return isprint(ch) && ch!=' ' && ch<128;
    }
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
    feature_recorder_set &fs = sp.fs;
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = WORDLIST;
        sp.info->flags = scanner_info::SCANNER_DISABLED;
        sp.info->get_config("word_min",&word_min,"Minimum word size");
        sp.info->get_config("word_max",&word_max,"Maximum word size");
        sp.info->get_config("max_word_outfile_size",&max_word_outfile_size,
                            "Maximum size of the words output file");
        sp.info->get_config("wordlist_use_flatfiles",&wordlist_use_flatfiles,"Override SQL settings and use flatfiles for wordlist");
        sp.info->get_config("strings",&strings,"Scan for strings instead of words");

        if(wordlist_use_flatfiles || fs.db3==0){
            sp.info->feature_names.insert(WORDLIST);
        }
        if(word_min>word_max){
            fprintf(stderr,"ERROR: word_min (%d) > word_max (%d)\n",word_min,word_max);
            exit(1);
        }
	wordchar_setup();
	return;
    }

    bool use_wordlist_recorder  = (wordlist_use_flatfiles || (fs.db3==0 && sp.fs.flag_notset(feature_recorder_set::DISABLE_FILE_RECORDERS)));
    feature_recorder *wordlist_recorder = use_wordlist_recorder ? fs.get_name(WORDLIST) : 0;


    /* init code is not multi-threaded */
    if(sp.phase==scanner_params::PHASE_INIT){
        if (wordlist_recorder) {
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_CONTEXT);      // not useful for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_STOPLIST);     // not necessary for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_ALERTLIST);    // not necessary for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_FEATURES_SQL); // SQL wordlist handled separately.
            return;
        }
        
#ifdef USE_SQLITE3
        if (fs.db3) {
            fs.db_send_sql(fs.db3,schema_wordlist);
            wordlist_stmt = new feature_recorder::besql_stmt(fs.db3,insert_statement);
            return;
        }
#endif
        assert(sp.fs.flag_set(feature_recorder_set::DISABLE_FILE_RECORDERS)); // this flag better be set
        return;
    }
        
    /* shutdown code is multi-threaded */
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
        if(!strings){
          std::cout << "Phase 3. Uniquifying and recombining wordlist\n";
          ofn_template = sp.fs.get_outdir()+"/wordlist_split_%03d.txt";

          if (wordlist_recorder) {
              wordlist_split_and_dedup(sp.fs.get_outdir()+"/" WORDLIST ".txt");
              return;
          }
        }

        if (fs.db3) {
            wordlist_sql_write(fs.db3);
            return;
        }
    }


    /* multi-threaded! */
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;

	/* Look for words in the buffer. Runs a finite state machine.
	 * for each character in the buffer. There are only two
	 * transitions that we care about:
         *
	 * case 1 - we are not in a word & this character starts a word.
         *
	 * case 2 - we are in a word & this character ends a word.
	 */
    
        if (sbuf.bufsize==0){           // nothing to scan
            return;
        }

        bool in_word     = false;        // true if we are in a word
        u_int wordstart = 0;            // if we are in the word, where it started
	for(u_int i=0; i<sbuf.bufsize; i++){
            bool is_wordchar = wordchar[sbuf.get8u(i)];
            if (!in_word) {
                /* case 1 - we are not in a word */
                /* does this character start a word? */
                if (is_wordchar){
                    in_word = true;
                    wordstart = i;
                }
                /* If we are not in a word and we have extended into the margin, quit. */
                if (i>sbuf.pagesize){
                    return;
                }
                continue;
	    }
            else {
                /* case 2 - we are in a word */
                /* If this is the end of the word, or we are in the word and this is the end of the margin,
                 * do end-of-word processing.
                 */
                /* Does - we are in a word & this character ends a word. */
                if (in_word && !is_wordchar){
                    uint32_t len = i - wordstart;
                    if ((word_min <= len) && (len <=  word_max)){
                        /* Save the word that starts at sbuf.buf+wordstart that has a length of len. */
                        std::string word = sbuf.substr(wordstart,len);
                    
#ifdef DEBUG_THIS
                        std::cerr << "word=" << word << " wr="
                                  << wordlist_recorder << " fs.db3=" << fs.db3 << "\n";
#endif
                        if (wordlist_recorder) {
                            wordlist_recorder->write(sbuf.pos0+wordstart,word,"");
                        } else if (fs.db3) {
#ifdef USE_SQLITE3
                            cppmutex::lock lock(wordlist_stmt->Mstmt);
                            sqlite3_bind_blob(wordlist_stmt->stmt, 1,
                                              (const char *)word.data(), word.size(), SQLITE_STATIC);
                            if (sqlite3_step(wordlist_stmt->stmt) != SQLITE_DONE) {
                                fprintf(stderr,"sqlite3_step failed on scan_wordlist\n");
                            }
                            sqlite3_reset(wordlist_stmt->stmt);
#endif
                        } 
                    }
                    wordstart = 0;
                    in_word = false;
                }
	    }
	}
    }
}
