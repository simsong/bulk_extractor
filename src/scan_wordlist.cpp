#include <cstdlib>
#include <cstring>
#include <cinttypes>

/*
 * This module creates a wordlist that can be used for password cracking.
 * To minimize memory pressure, the wordlist is created in two passes.
 * Pass 1 - During scanning (phase 1), each word found is written to a word.
 * Pass 2 - the words are read, uniquified, and written out in sorted order,
 *          shortest to longest, in files no longer than 100MB each.
 *          This is designed for Elcomsoft's tools, but you may have success
 *          with others.
 *
 * In BE1.6 we also had the ability to use SQLite3 for making the password list.
 * We haven't gotten that working here yet.
 */



#include "config.h"
#include "be20_api/utils.h"
#include "be20_api/scanner_params.h"
#include "be20_api/scanner_set.h"
#include "scan_wordlist.h"

#if defined(HAVE_LIBSQLITE3) && defined(HAVE_SQLITE3_H)
#define USE_SQLITE3
#endif

bool wordlist_strings = false;

/* Haven't figured out the SQL interface yet. */
#if 0
#ifdef USE_SQLITE3
static const char *schema_wordlist[] = {
    "CREATE TABLE wordlist (word BLOB)",
    "CREATE UNIQUE INDEX wordlist_i on wordlist(word)",
    0};
static const char *insert_statement = "INSERT OR IGNORE INTO wordlist VALUES (?);";
static feature_recorder::besql_stmt *wordlist_stmt=0;
static const char *select_statement = "SELECT DISTINCT word FROM wordlist ORDER BY length(word),word";
#endif
#endif


Scan_Wordlist::Scan_Wordlist(scanner_params &sp, bool strings_):strings(strings_)
{
    // Build the truth table for the wordlist separator values.
    for(int ch=0; ch<256; ch++){
        if (strings) {
            wordchar[ch] = isprint(ch) && ch<128;
        } else {
            wordchar[ch] = isprint(ch) && ch!=' ' && ch<128;
        }
    }
}

void Scan_Wordlist::process_sbuf(scanner_params &sp)
{
    const sbuf_t &sbuf = *sp.sbuf;
    if (flat_wordlist==nullptr) {
        flat_wordlist = &sp.named_feature_recorder("wordlist");
    }

    /* Simplified word extractor. It's good enough to pull out stuff for cryptanalysis. */
    bool in_word     = false;        // true if we are in a word
    size_t wordstart = 0;            // if we are in the word, where it started
    for(size_t i=0; i<sbuf.bufsize; i++){
        bool is_wordchar = wordchar[sbuf[i]];
        if (!in_word) {
            /* case 1 - we are not in a word */
            /* does this character start a word? */
            if (is_wordchar){
                in_word = true;
                wordstart = i;
            }
            /* If we are not in a word and we have extended into the margin, quit. */
            if (i > sbuf.pagesize){
                return;
            }
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

                    /* Save the word that starts at sbuf.buf+wordstart that has a length of len.
                     * Do we need to keep the position? It might be useful in some applications.
                     */
                    std::string word = sbuf.substr(wordstart,len);
                    flat_wordlist->write(sbuf.pos0+wordstart, word, "");

                    /* check for (word), <word>, and [word] */
                    if (word.size()>2 && word[0]=='(' && word[word.size()-1]==')') {
                        flat_wordlist->write(sbuf.pos0+wordstart+1, word.substr(1,word.size()-2), "");
                    }
                    if (word.size()>2 && word[0]=='<' && word[word.size()-1]=='>') {
                        flat_wordlist->write(sbuf.pos0+wordstart+1, word.substr(1,word.size()-2), "");
                    }
                    if (word.size()>2 && word[0]=='[' && word[word.size()-1]==']') {
                        flat_wordlist->write(sbuf.pos0+wordstart+1, word.substr(1,word.size()-2), "");
                    }
                }
#if 0
#ifdef USE_SQLITE3
                /* Figure out how to write the wordlist to the SQL later. */
                if (fs.db) {
                    const std::lock_guard<std::mutex> lock(wordlist_stmt->Mstmt);
                    sqlite3_bind_blob(wordlist_stmt->stmt, 1,
                                      (const char *)word.data(), word.size(), SQLITE_STATIC);
                    if (sqlite3_step(wordlist_stmt->stmt) != SQLITE_DONE) {
                        fprintf(stderr,"sqlite3_step failed on scan_wordlist\n");
                    }
                    sqlite3_reset(wordlist_stmt->stmt);
                }
#endif
#endif
                wordstart = 0;
                in_word = false;
            }
        }
    }
}

void Scan_Wordlist::dump_seen_wordlist()
{
    /* Dump the words so far */
    for(const auto &it : seen_wordlist){
        if (it.size()==0) continue;
        if (wordlist_out == nullptr ){
            auto wordlist_segment_path = flat_wordlist->fname_in_outdir("dedup", wordlist_segment++);
            wordlist_out = new std::ofstream( wordlist_segment_path );
            if (!wordlist_out->is_open()) {
                throw std::runtime_error("cannot open: " + wordlist_segment_path.string());
            }
        }
        (*wordlist_out) << it << "\n";
    }
    if (wordlist_out != nullptr) {
        wordlist_out->close();
        delete wordlist_out;
        wordlist_out = nullptr;
    }
    seen_wordlist.clear();
}


void Scan_Wordlist::shutdown(scanner_params &sp)
{
    // if `strings` is set, report all strings, not words, so no shutdown.
    if (strings) {
        return;
    }
    flat_wordlist = &sp.named_feature_recorder("wordlist");

    flat_wordlist->flush();
    auto feature_recorder_path = flat_wordlist->fname_in_outdir("", feature_recorder::NO_COUNT);
    std::ifstream f2( feature_recorder_path );
    if (!f2.is_open()) {
        throw std::runtime_error(std::string("Scan_Wordlist::shutdown: Cannot open ")+feature_recorder_path.string());
    }

    /* Read all of the words and uniquify them */
    uint64_t outfilesize = 0;
    while(!f2.eof()){
        std::string line;
        getline(f2,line);
        if (line[0]=='#') continue;	                // ignore comments
        size_t t1 = line.find('\t');		// find the beginning of the feature
        if (t1!=std::string::npos) line = line.substr(t1+1);

        // The end of the feature is the end of the line, since we did not write the context
        const std::string &word = line;

        // Insert into the hash list. If we ran out of space, dump it all and restart.
        try {
            if (word.size()>0) seen_wordlist.insert(word);
        }
        catch (std::bad_alloc &er) {
            std::cerr << er.what() << std::endl;
            std::cerr << "scan_wordlist:bad_alloc: Dumping current dataset; will then restart dedup." << std::endl;
            dump_seen_wordlist();
            outfilesize = 0;
        }

        if (outfilesize > max_output_file_size ) {
            dump_seen_wordlist();
            outfilesize = 0;
        }
    }
    dump_seen_wordlist();
    f2.close();
}


/* Note that this is a singleton. Would be useful to have a mehcanism for per-invocation */
Scan_Wordlist *wordlist = nullptr;

extern "C"
void scan_wordlist(scanner_params &sp)
{
    bool wordlist_use_flatfiles = true;

    if (sp.phase==scanner_params::PHASE_INIT){
        uint32_t word_min = Scan_Wordlist::WORD_MIN_DEFAULT;
        uint32_t word_max = Scan_Wordlist::WORD_MAX_DEFAULT;
        uint64_t max_output_file_size = Scan_Wordlist::MAX_OUTPUT_FILE_SIZE;
        sp.check_version();
        sp.info->set_name("wordlist" );
        sp.info->scanner_flags.default_enabled = false; // = scanner_info::SCANNER_DISABLED;
        sp.get_scanner_config("word_min",&word_min,"Minimum word size");
        sp.get_scanner_config("word_max",&word_max,"Maximum word size");
        sp.get_scanner_config("max_output_file_size",&max_output_file_size, "Maximum size of the words output file");
        //sp.get_scanner_config("wordlist_use_flatfiles",&wordlist_use_flatfiles,"Use flatfiles for wordlist");
        //sp.get_scanner_config("wordlist_use_sql",&wordlist_use_sql,"Use SQL DB for wordlist");
        sp.get_scanner_config("strings",&wordlist_strings,"Scan for strings instead of words");

        if (wordlist_use_flatfiles){
            auto def = feature_recorder_def(Scan_Wordlist::WORDLIST);
            def.flags.no_context   = true;
            def.flags.no_stoplist  = true;
            def.flags.no_alertlist = true;
            sp.info->feature_defs.push_back( def );
        }
        if (word_min > word_max){
            std::cerr << "ERROR: word_min (" << word_min << ") > word_max (" << word_max << ")\n";
            throw std::runtime_error("word_min > word_max");
        }
        wordlist = new Scan_Wordlist(sp, wordlist_strings);
        wordlist->word_min = word_min;
        wordlist->word_max = word_max;
        wordlist->max_output_file_size = max_output_file_size;

#if 0
#ifdef USE_SQLITE3
    TODO: Do something to send the SQL through
        if (fs.db) {
            fs.db_send_sql(fs.db,schema_wordlist);
            wordlist_stmt = new feature_recorder::besql_stmt(fs.db,insert_statement);
            return;
        }
#endif
#endif
    }

    if (sp.phase==scanner_params::PHASE_SCAN){
        assert (wordlist!=nullptr);
        wordlist->process_sbuf(sp);
    }

    if (sp.phase==scanner_params::PHASE_SHUTDOWN){
        assert (wordlist!=nullptr);
        wordlist->shutdown(sp);
    }
    if (sp.phase==scanner_params::PHASE_CLEANUP){
        if (wordlist) {
            delete wordlist;
            wordlist = nullptr;
        }
    }
}


#if 0
/* Similar to above; write out the wordlist using SQL.
 * Not-multi-threaded, but threadsafe nonetheless.
 */
static void wordlist_sql_write(sqlite3 *db)
{
#ifdef USE_SQLITE3
    feature_recorder::besql_stmt s(db,select_statement);
    while (sqlite3_step(s.stmt) != SQLITE_DONE) {
        const char *base = (const char *)sqlite3_column_blob(s.stmt,0);
        int len          = sqlite3_column_bytes(s.stmt,0);
        wordlist_write_word(std::string(base,len));
    }
    if (of2.is_open()) of2.close();
#endif
}
#endif
