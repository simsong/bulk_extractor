#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint32_t word_min = 6;
static uint32_t word_max = 14;
static uint64_t max_word_outfile_size=100*1000*1000;
static bool wordlist_use_sql = false;

/* Wordlist support for flat files */
class WordlistSorter {
public:
    bool operator()(const std::string &a,const std::string &b) {
	if(a.size() < b.size()) return true;
	if(a.size() > b.size()) return false;
	return a<b;
    }
};
#define WORDLIST "wordlist"

/* wordlist support for SQL */
static const char *schema_wordlist[] = {
    "CREATE TABLE wordlist (word BLOB)",
    "CREATE UNIQUE INDEX wordlist_i on wordlist(word)",
    0};

static const char *insert_statement = "INSERT OR IGNORE INTO wordlist VALUES (?);";
static feature_recorder::besql_stmt *stmt=0;
static const char *select_statement = "SELECT DISTINCT word FROM wordlist ORDER BY length(word),word";


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

static int callback_printer(void *param, int argc, char **argv, char **azColName)
{
    wordlist_write_word(argv[0]);
    return 0;
}


/* Similar to above; write out the wordlist using SQL */
static void wordlist_sql_write(BEAPI_SQLITE3 *db3)
{
    char *errmsg = 0;
    if (sqlite3_exec(db3,select_statement,callback_printer,0,&errmsg)){
        std::cerr << "sqlite3: " << errmsg << "\n";
    }
    if(of2.is_open()) of2.close();
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
        sp.info->name  = WORDLIST;
        sp.info->flags = scanner_info::SCANNER_DISABLED;
        sp.info->get_config("word_min",&word_min,"Minimum word size");
        sp.info->get_config("word_max",&word_max,"Maximum word size");
        sp.info->get_config("max_word_outfile_size",&max_word_outfile_size,
                            "Maximum size of the words output file");
        sp.info->get_config("wordlist_use_sql",&wordlist_use_sql,"write wordlist to SQL database");
        if ( !wordlist_use_sql ){
            sp.info->feature_names.insert(WORDLIST);
        }
        if(word_min>word_max){
            fprintf(stderr,"ERROR: word_min=%d word_max=%d\n",word_min,word_max);
            exit(1);
        }
	wordchar_setup();
	return;
    }
    feature_recorder_set &fs = sp.fs;

    feature_recorder *wordlist_recorder = (fs.db3 && wordlist_use_sql) ? 0 : fs.get_name(WORDLIST);

    if(sp.phase==scanner_params::PHASE_INIT){
        if (fs.db3 && wordlist_use_sql){
            fs.db_send_sql(fs.db3,schema_wordlist);
            stmt = new feature_recorder::besql_stmt(fs.db3,insert_statement);
        } else {
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_CONTEXT);  // not useful for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_STOPLIST); // not necessary for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_ALERTLIST); // not necessary for wordlist
            wordlist_recorder->set_flag(feature_recorder::FLAG_NO_FEATURES_SQL); // SQL wordlist handled separately.
        }
        return;
    }
        
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
        std::cout << "Phase 3. Uniquifying and recombining wordlist\n";
        ofn_template = sp.fs.get_outdir()+"/wordlist_split_%03d.txt";
        if (fs.db3 && wordlist_use_sql){
            wordlist_sql_write(fs.db3);
        } else {
            wordlist_split_and_dedup(sp.fs.get_outdir()+"/" WORDLIST ".txt");
        }
	return;
    }
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
    
	int wordstart = -1;			// >=0 means we are in a word
	for(u_int i=0; i<sbuf.pagesize; i++){

	    /* case 1 - we are not in a word & this character starts a word. */
	    bool iswordchar = wordchar[sbuf.buf[i]];
	    if(wordstart<0 && iswordchar && i!=sbuf.pagesize-1){
		wordstart = i;
		continue;
	    }
            /* case 2 - we are in a word & this character ends a word. */
	    if(wordstart>=0 && (!iswordchar || i==sbuf.pagesize-1)){
		uint32_t len = i-wordstart;
		if((word_min <= len) && (len <=  word_max)){

                    /* Save the word that starts at sbuf.buf+wordstart that has a length of len. */
                    
                    if(fs.db3 && wordlist_use_sql){
                        sqlite3_bind_blob(stmt->stmt, 1, (const char *)sbuf.buf+wordstart, len, SQLITE_STATIC);
                        if (sqlite3_step(stmt->stmt) != SQLITE_DONE) {
                            fprintf(stderr,"sqlite3_step failed on scan_wordlist\n");
                        }
                        sqlite3_reset(stmt->stmt);
                    } else {
                        wordlist_recorder->write_buf(sbuf,wordstart,len);
                    }
		}
		wordstart = -1;
	    }
	}
    }
}
