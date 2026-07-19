/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "formatter.h"

/**
 * write() is the main entry point for writing a feature at a given position with context.
 * write() checks the stoplist and escapes non-UTF8 characters, then calls write0().
 */
void feature_recorder::write(const pos0_t& pos0, const std::string& feature_, const std::string& context_) {
    if (fs.flags.disabled) return; // disabled
    if (fs.flags.pedantic) {
        if (feature_.size() > def.max_feature_size) {
            throw std::runtime_error(Formatter() << "feature_recorder::write : feature_.size()=" << feature_.size());
        }
        if (context_.size() > def.max_context_size) {
            throw std::runtime_error(Formatter() << "feature_recorder::write : context_.size()=" << context_.size());
        }
    }

    std::string feature = feature_;
    std::string context = flags.no_context ? "" : context_;
    std::string* feature_utf8 = AtomicUnicodeHistogram::make_utf8(feature); // a utf8 feature

    quote_if_necessary(feature, context);

    if (feature.size() == 0 && fs.flags.pedantic) {
        throw std::runtime_error(Formatter()  name << ": zero length feature at " << pos0);
    }

    /* First check to see if the feature is on the stop list.
     * Only do this if we have a stop_list_recorder (the stop list recorder itself
     * does not have a stop list recorder. If it did we would infinitely recurse.
     */
    if (flags.no_stoplist == false && fs.stop_list && fs.stop_list_recorder &&
        fs.stop_list->check_feature_context(*feature_utf8, context)) {
        fs.stop_list_recorder->write(pos0, feature, context);
        delete feature_utf8;
        return;
    }

    /* The alert list is a special features that are called out.
     * If we have one of those, write it to the redlist.
     */
#if 0
    if (flags.no_alertlist==false
        && fs.alert_list
        && fs.alert_list->check_feature_context(*feature_utf8,context)) {
        std::string alert_fn = fs.get_outdir() + "/ALERTS_found.txt";
        const std::lock_guard<std::mutex> lock(Mr);                // notice we are locking the alert list
        std::ofstream rf(alert_fn.c_str(),std::ios_base::app);
        if(rf.is_open()){
            rf << pos0.shift(fs.offset_add).str() << '\t' << feature << '\t' << "\n";
        }
    }
#endif

#if 0
    /* Support in-memory histograms */
    for (const auto &it:mhistograms ){
        const histogram_def &def = it.first;
        mhistogram_t *m = it.second;
        std::string new_feature = *feature_utf8;
        if (def.require.size()==0 || new_feature.find_first_of(def.require)!=std::string::npos){
            /* If there is a pattern to use, use it to simplify the feature */
            if (def.pattern.size()){
                std::smatch sm;
                std::regex_search( new_feature, sm, def.reg);
                if (sm.size() == 0){
                    // no search match; avoid this feature
                    new_feature = "";
                }
                else {
                    new_feature = sm.str();
                }
            }
            if(new_feature.size()) m->add(new_feature,1);
        }
    }
#endif

    /* Finally write out the feature and the context */
    this->write0(pos0, feature, context);
    delete feature_utf8;
}

/**
 * Given a buffer, an offset into that buffer of the feature, and the length
 * of the feature, make the context and write it out. This is mostly used
 * for writing from within the lexical analyzers.
 */

void feature_recorder::write_buf(const sbuf_t& sbuf, size_t pos, size_t len) {
    /* If we are in the margin, ignore; it will be processed again */
    if (pos >= sbuf.pagesize && pos < sbuf.bufsize) { return; }

    if (pos >= sbuf.bufsize) { /* Sanity checks */
        std::cerr << "*** write_buf: WRITE OUTSIDE BUFFER. "
                  << " pos=" << pos << " sbuf=" << sbuf << "\n";
        return;
    }

    /* Asked to write beyond bufsize; bring it in */
    if (pos + len > sbuf.bufsize) { len = sbuf.bufsize - pos; }

    std::string feature = sbuf.substr(pos, len);
    std::string context;

    if (flags.no_context == false) {
        /* Context write; create a clean context */
        size_t p0 = context_window < pos ? pos - context_window : 0;
        size_t p1 = pos + len + context_window;

        if (p1 > sbuf.bufsize) p1 = sbuf.bufsize;
        assert(p0 <= p1);
        context = sbuf.substr(p0, p1 - p0);
    }
    this->write(sbuf.pos0 + pos, feature, context);
}
