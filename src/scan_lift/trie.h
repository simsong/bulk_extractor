//#include "myheader.h"
#ifndef _TERM_WEIGHTING_H
#define _TERM_WEIGHTING_H

namespace Utils {
  using namespace std;

  struct Trie {
    int inserted_count, nodes_used;
    int *id;
    map<char,int> *T;

    Trie( const int& memory );
    int find( const char *str );
    int insert( const char *str );
    ~Trie();
  };	
}

#endif
