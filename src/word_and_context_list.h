#ifndef WORD_AND_CONTEXT_LIST_H
#define WORD_AND_CONTEXT_LIST_H

#include "beregex.h"

/**
 * \addtogroup internal_interfaces
 * @{
 * \file
 * word_and_context_list:
 *
 * A re-implementation of the basic stop list, regular expression
 * stop_list, and context-sensitive stop list.
 *
 * Method:
 * Each entry in the stop list can be represented as:
 * - a feature that is stopped, with optional context.
 * - a regular expression
 * 
 * Context is represented as a string before the feature and a string after.
 * 
 * The stop list contains is a map of features that are stopped. 
 * For each feature, there may be no context or a list of context. 
 * If there is no context and the feature is in the list, 
 */

/*
 * context is a class that records the feature, the text before, and the text after.
 * Typically this is used for stop lists and alert lists. 
 */

#include <tr1/unordered_map>
#include <tr1/unordered_set>

class context {
public:
    static void extract_before_after(const string &feature,const string &ctx,string &before,string &after){
	if(feature.size() <= ctx.size()){
	    /* The most simple algorithm is a sliding window */
	    for(size_t i = 0;i<ctx.size() - feature.size();i++){
		if(ctx.substr(i,feature.size())==feature){
		    before = ctx.substr(0,i);
		    after  = ctx.substr(i+feature.size());
		    return;
		}
	    }
	}
	before.clear();			// can't be done
	after.clear();
    }

    // constructors to make a context with nothing before or after, with just a context, or with all three
    context(const string &f):feature(f),before(),after(){}
    context(const string &f,const string &c):feature(f),before(),after(){
	extract_before_after(f,c,before,after);
    }
    context(const string &f,const string &b,const string &a):feature(f),before(b),after(a){}
    string feature;
    string before;
    string after;
};

inline std::ostream & operator <<(std::ostream &os,const class context &c)
{
    os << "context[" << c.before << "|" << c.feature  << "|" << c.after << "]";
    return os;
}
inline bool operator ==(const class context &a,const class context &b)
{
    return (a.feature==b.feature) && (a.before==b.before) && (a.after==b.after);
}

/**
 * the object that holds the word and context list
 */
class word_and_context_list {
private:
    typedef tr1::unordered_multimap<string,context> stopmap_t;
    stopmap_t fcmap;			// maps features to contexts; for finding them

    typedef tr1::unordered_set< string > stopset_t;
    stopset_t context_set;			// presence of a pair in fcmap

    beregex_vector patterns;
public:
    /**
     * rstrcmp is like strcmp, except it compares strings right-aligned
     * and only compares the minimum sized string of the two.
     */
    static int rstrcmp(const string &a,const string &b);

    word_and_context_list():fcmap(),context_set(),patterns(){ }
    ~word_and_context_list(){
	for(beregex_vector::iterator it=patterns.begin(); it != patterns.end(); it++){
	    delete *it;
	}
    }
    size_t size(){ return fcmap.size() + patterns.size();}
    void add_regex(const string &pat);	// not threadsafe
    bool add_fc(const string &f,const string &c); // not threadsafe
    int readfile(const string &fname);	// not threadsafe

    // return true if the probe with context is in the list or in the stopmap
    bool check(const string &probe,const string &before, const string &after) const; // threadsafe
    bool check_feature_context(const string &probe,const string &context) const; // threadsafe
    void dump();
};


inline int word_and_context_list::rstrcmp(const string &a,const string &b)
{
    size_t alen = a.size();
    size_t blen = b.size();
    size_t len = min(alen,blen);
    for(size_t i=0;i<len;i++){
	size_t apos = alen - len + i;
	size_t bpos = blen - len + i;
	if(a[apos] < b[bpos]) return -1;
	if(a[apos] > b[bpos]) return 1;
    }
    return 0;
}

#endif
