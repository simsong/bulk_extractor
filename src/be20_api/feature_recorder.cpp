/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

// not present under mingw:
//#include <sys/times.h>

#include <ctime>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

#include "feature_recorder.h"
#include "feature_recorder_set.h"
#include "formatter.h"
#include "unicode_escape.h"
#include "utils.h"
#include "word_and_context_list.h"


/* These are all overridden in the subclass */
feature_recorder::feature_recorder(class feature_recorder_set& fs_, const struct feature_recorder_def def_)
    : fs(fs_), name(def_.name), def(def_)
{
    debug_histograms               = getenv_debug(DEBUG_HISTOGRAMS_ENV);
    disable_incremental_histograms = getenv_debug(DEBUG_HISTOGRAMS_NO_INCREMENTAL_ENV);
}

/* This is here for the vtable */
feature_recorder::~feature_recorder() { }

/* This doesn't need to be overridden, but it can be */
void feature_recorder::flush() {}


const std::filesystem::path feature_recorder::get_outdir() const // cannot be inline becuase it accesses fs
{
    return fs.get_outdir();
}

/*
 * Returns a filename for this feature recorder with a specific suffix.
 */
const std::filesystem::path feature_recorder::fname_in_outdir(std::string suffix, int count) const {
    if (fs.get_outdir() == scanner_config::NO_OUTDIR) {
        throw std::runtime_error("fname_in_outdir called, but outdir==NO_OUTDIR");
    }

    const std::lock_guard<std::mutex> lock(Mcarve); // grab the carving mutex

    std::filesystem::path base_path = fs.get_outdir() / this->name;
    if (suffix.size() > 0) { base_path = base_path.string() + "_" + suffix; }
    if (count == NO_COUNT) return base_path.string() + ".txt";
    if (count != NEXT_COUNT) { return base_path.string() + "_" + std::to_string(count) + ".txt"; }

    /* Probe for a file that we can create. When we create it, return the name.
     * This does not create a TOCTOU error because we have the carving mutex
     */
    for (int i = 0; i < 1000000; i++) {
        std::filesystem::path fname = base_path.string() + (i > 0 ? "_" + std::to_string(i) : "") + ".txt";
        if (!std::filesystem::exists( fname )){
            std::ofstream f(fname, std::fstream::out | std::fstream::trunc);
            if (!f.is_open()) {
                throw std::runtime_error(Formatter() << "cannot create: " << fname);
            }
            f.close();
            return fname;
        }
    }
    throw std::runtime_error("It is unlikely that there are a million files, "
                             "so this is probably a logic error.");
}

/**
 * the main entry point of writing a feature and its context to the feature file.
 * processes the stop list
 */

void feature_recorder::quote_if_necessary(std::string& feature, std::string& context) const {
    /* By default quote string that is not UTF-8, and quote backslashes. */
    bool escape_bad_utf8 = true;
    bool escape_backslash = true;

    if (def.flags.no_quote) { // don't quote either
        escape_bad_utf8 = false;
        escape_backslash = false;
    }

    if (def.flags.xml) { // only quote bad utf8
        escape_bad_utf8 = true;
        escape_backslash = false;
    }

    feature = validateOrEscapeUTF8(feature, escape_bad_utf8, escape_backslash, validateOrEscapeUTF8_validate);

    if (feature.size() > def.max_feature_size) { feature.resize(def.max_feature_size); }

    if (def.flags.no_context == false) {
        context = validateOrEscapeUTF8(context, escape_bad_utf8, escape_backslash, validateOrEscapeUTF8_validate);
        if (context.size() > def.max_context_size) { context.resize(def.max_context_size); }
    }
}

/*
 * write0:
 */
void feature_recorder::write0(const std::string& str) {}

/*
 * Write: keep track of count of features written.
 */
void feature_recorder::write0(const pos0_t& pos0, const std::string& feature, const std::string& context) {
    if (fs.flags.disabled) { return; }
    features_written += 1;
}

/**
 * write() is the main entry point for writing a feature at a given position with context.
 * Activities:
 * - checks the stoplist.
 * - escapes non-UTF8 characters and calls write0().
 * - Takes the original feature and sends to the histogram system.
 *
 * This is not overwritten by subclasses.
 */
void feature_recorder::write(const pos0_t& pos0, const std::string &original_feature, const std::string &original_context) {
    if (fs.flags.disabled) return; // disabled

    /* The 'pedantic' flag is meant for debugging scanners and feature recorders, not for production use. */
    if (fs.flags.pedantic) {
        if (original_feature.size() > def.max_feature_size) {
            throw std::runtime_error(std::string("feature_recorder::write : feature_.size()=") +
                                     std::to_string(original_feature.size()));
        }
        if (original_context.size() > def.max_context_size) {
            throw std::runtime_error(std::string("feature_recorder::write : context_.size()=") +
                                     std::to_string(original_context.size()));
        }
    }

    /* TODO: This needs to be change to do all processing in utf32 and not utf8 */

    std::string feature_utf8 = make_utf8( original_feature );
    std::string context      = def.flags.no_context ? "" : original_context;

    quote_if_necessary(feature_utf8, context);

    if (feature_utf8.size() == 0) {
        std::cerr << name << ": zero length feature at " << pos0 << "\n";
        if (fs.flags.pedantic) {
            throw std::runtime_error(std::string("zero length feature at") + pos0.str());
        }
        return;
    }

    /* First check to see if the feature is on the stop list.
     * Only do this if we have a stop_list_recorder (the stop list recorder itself
     * does not have a stop list recorder. If it did we would infinitely recurse.
     */
    if (def.flags.no_stoplist == false
        && fs.stop_list
        && fs.stop_list_recorder
        && fs.stop_list->check_feature_context(feature_utf8, context)) {
        fs.stop_list_recorder->write(pos0, feature_utf8, context);
        return;
    }

    /* The alert list is a special features that are called out.
     * If we have one of those, write it to the redlist.
     */
#if 0
    if (flags.no_alertlist==false
        && fs.alert_list
        && fs.alert_list->check_feature_context(*feature_utf8,context)) {
        std::filesystem::path alert_fn = fs.get_outdir() + "/ALERTS_found.txt";
        const std::lock_guard<std::mutex> lock(Mr);                // notice we are locking the alert list
        std::ofstream rf(alert_fn.c_str(),std::ios_base::app);
        if(rf.is_open()){
            rf << pos0.shift(fs.offset_add).str() << '\t' << feature << '\t' << "\n";
        }
    }
#endif

    /* Write out the feature and the context */
    this->write0(pos0, feature_utf8, context);

    /* Add the feature to any histograms.
     * histograms_add_feature tracks whether it is getting utf-8 or utf-16 string.
     *
     * Note that this means that the utf-16 determination has to be made twice.
     * Oh well.
     *
     * In the feature_recorder_file this will be subclasses.
     * In the feature_recorder_sql, it is a NOOP.
     */
    if (disable_incremental_histograms == false ){
        this->histograms_incremental_add_feature_context(original_feature, original_context);
    }

}

/**
 * Given a buffer, an offset into that buffer of the feature, and the length
 * of the feature, make the context and write it out. This is mostly used
 * for writing from within the lexical analyzers.
 */

void feature_recorder::write_buf(const sbuf_t& sbuf, size_t pos, size_t len) {
    if (fs.flags.debug) {
        std::cerr << "*** write_buf " << name << " sbuf=" << sbuf << " pos=" << pos << " len=" << len << "\n";
        /*
         * for debugging, you can set a debug at Breakpoint Reached below.
         * This lets you fast-forward to the error condition without having to set a watch point.
         */
        if (sbuf.pos0 == debug_halt_pos0 || pos == debug_halt_pos) { std::cerr << "Breakpoint Reached.\n"; }
    }

    /* If we are in the margin, ignore; it will be processed again */
    if (pos >= sbuf.pagesize && pos < sbuf.bufsize) { return; }

    if (pos >= sbuf.bufsize) { /* Sanity checks */
        throw std::runtime_error(Formatter() << "*** write_buf: WRITE OUTSIDE BUFFER. pos=" << std::to_string(pos)  << " sbuf=" << sbuf);
    }

    /* Asked to write beyond bufsize; bring it in */
    if (pos + len > sbuf.bufsize) { len = sbuf.bufsize - pos; }

    std::string feature = sbuf.substr(pos, len);
    std::string context;

    if (def.flags.no_context == false) {
        /* Context write; create a clean context */
        size_t p0 = context_window < pos ? pos - context_window : 0;
        size_t p1 = pos + len + context_window;

        if (p1 > sbuf.bufsize) p1 = sbuf.bufsize;
        assert(p0 <= p1);
        context = sbuf.substr(p0, p1 - p0);
    }
    this->write(sbuf.pos0 + pos, feature, context);
}

/**
 * replace a character in a string with another
 */
std::string replace(const std::string& src, char f, char t) {
    std::string ret;
    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == f)
            ret.push_back(t);
        else
            ret.push_back(src[i]);
    }
    return ret;
}

/****************************************************************
 *** carving support
 ****************************************************************
 *
 * carving support.
 * 2021-07-10 - we no longer carve to forensic path or actual path. Now we carve sequential numbers.
 * 2014-04-24 - $ is no longer valid either
 * 2013-08-29 - replace invalid characters in filenames
 * 2013-07-30 - automatically bin directories
 * 2013-06-08 - filenames are the forensic path.
 */

std::string feature_recorder::sanitize_filename(std::string in)
{
    std::string out;
    for (size_t i = 0; i < in.size(); i++) {
        uint8_t ch = in.at(i);
        if (ch <= 32 || ch >= 128 || ch == '"' || ch == '*' || ch == '+' || ch == ',' || ch == '/' || ch == ':' ||
            ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '\\' || ch == '[' || ch == ']' ||
            ch == '|' || ch == '$') {
            out.push_back('_');
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

/*
 * write buffer to specified dirname/filename for writing data.
 * The name is {outdir}/{scanner}/{seq}/{pos0}.{ext}
 * Where: {outdir} is the output directory of the feature recorder.
 *        {scanner} is the name of the scanner
 *        {seq} is 000 through 999.  (1000 files per directory)
 *        {pos0} is where the feature was found.
 *        {ext} is the provided extension.

 * @param sbuf   - the buffer to carve
 * @param pos    - offset in the buffer to carve
 * @param len    - how many bytes to carve
 * @return - the path relative to the outdir
 */
/* Carving to a file depends on the carving mode.
 *
 * The purpose of CARVE_ENCODED is to allow us to carve JPEGs when
 * they are embedded in, say, GZIP files, but not carve JPEGs that
 * are bare.  The difficulty arises when you have a tool that can
 * go into, say, ZIP files. In this case, we don't want to carve
 * every ZIP file, just the (for example) XORed ZIP files. So the
 * ZIP carver doesn't carve every ZIP file, just the ZIP files
 * that are in HIBER files.  That is, we want to not carve a path
 * of ZIP-234234 but we do want to carve a path of
 * 1000-HIBER-33423-ZIP-2343.  This is implemented by having an
 * do_not_carve_encoding. the ZIP carver sets it to ZIP so it
 * won't carve things that are just found in a ZIP file. This
 * means that it won't carve disembodied ZIP files found in
 * unallocated space. You might want to do that.  If so, set ZIP's
 * carve mode to CARVE_ALL.
 */

#include <iomanip>
std::string feature_recorder::carve(const sbuf_t& header, const sbuf_t& data, std::string ext, time_t mtime)
{
    switch (carve_mode) {
    case feature_recorder_def::CARVE_NONE:
        return NO_CARVED_FILE; // carve nothing
    case feature_recorder_def::CARVE_ENCODED:
        if (data.pos0.path.size() == 0) return NO_CARVED_FILE; // not encoded
        if (data.pos0.alphaPart() == do_not_carve_encoding)
            return std::string(); // ignore if it is just encoded with this
        break;                    // otherwise carve
    case feature_recorder_def::CARVE_ALL:
        break;
    }

    /* See if we have previously carved this object, in which case do not carve it again */
    std::string carved_hash_hexvalue = hash(data);
    std::string carved_relative_path; // carved path reported in feature file, relative to outdir
    std::filesystem::path carved_absolute_path; // used for opening
    bool in_cache = carve_cache.check_for_presence_and_insert(carved_hash_hexvalue);

    if (in_cache) {
        carved_relative_path = CACHED;
    } else {
        /* Determine the directory and filename */
        int64_t myfileNumber = carved_file_count++; // atomic operation
        std::ostringstream seq;
        seq << std::setw(3) << std::setfill('0') << int(myfileNumber / 1000);
        const std::string thousands{seq.str()};

        /* Create the directory.
         * If it exists, the call fails.
         * (That's faster than checking to see if it exists and then creating the directory)
         */
        std::filesystem::create_directory(fs.get_outdir() / name);
        std::filesystem::create_directory(fs.get_outdir() / name / thousands);

        std::string fname  = data.pos0.str() + ext;
        auto rpos = fname.rfind('/');   // see if there is a '/' in the string
        if (rpos != std::string::npos ){
            fname = fname.substr(rpos+1); // remove everything up to the last /
        }
        fname  = sanitize_filename(fname);

        // carved relative path goes in the feature file
        carved_relative_path = name + "/" + thousands + "/" + fname;

        // carved absolute path is used for actually opening
        carved_absolute_path = fs.get_outdir() / name / thousands / fname;

        // fname_feature = fname.string().substr(fs.get_outdir().string().size()+1);
    }

    // write to the feature file
    std::stringstream xml;
    xml.str(std::string()); // clear the stringstream
    xml << "<fileobject>";
    if (!in_cache) xml << "<filename>" << carved_relative_path << "</filename>";
    xml << "<filesize>" << header.bufsize + data.bufsize << "</filesize>";
    xml << "<hashdigest type='" << fs.hasher.name << "'>" << carved_hash_hexvalue << "</hashdigest>";
    xml << "</fileobject>";
    this->write(data.pos0, carved_relative_path, xml.str());

    if (!in_cache) {
        /* Write the data */
        std::ofstream os;
        os.open(carved_absolute_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!os.is_open()) {
            throw feature_recorder::DiskWriteError(Formatter() << "cannot open file for writing:" << carved_absolute_path << ":" << std::strerror(errno));
        }
        header.write(os);
        data.write(os);
        os.close();
        if (os.bad()) {
            throw feature_recorder::DiskWriteError(Formatter() << "error writing file " << carved_absolute_path);
        }

        /* Set timestamp if necessary. Note that we do not use std::filesystem::last_write_time()
         * as there seems to be no portable way to use it under C++17.
         */
        if (mtime > 0) {
#ifdef HAVE_UTIMES
            const struct timeval times[2] = {{mtime, 0}, {mtime, 0}};
            utimes(carved_absolute_path.c_str(), times);
#endif
        }
    }
    return carved_relative_path;
}

const std::string feature_recorder::hash(const sbuf_t& sbuf) const
{
    return sbuf.hash(fs.hasher.func);
}

void feature_recorder::shutdown()
{
}

/**
 * flush the largest histogram to the disk. This is a way to release
 * allocated memory.
 *
 * In BE2.0, the file recorder's histograms are built in memory. If
 * they are too big for memory, file-histograms they are flushed and
 * need to be recombined in post-processing. SQL feature recorder uses the
 * SQLite3 to create the histograms.
 */
