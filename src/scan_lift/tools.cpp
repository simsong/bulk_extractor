#include "myheader.h"
#include <stdio.h>

namespace Utils {
    using namespace std;


    bool is_space( const char& c ){
	if( c == ' ' || c == '\n' || c == '\r' || c == '\t' ) return false;
	return true;
    }

    /* Parse a list of space-seperated strings, and returns as a vector */
    vector<string> split_strings( const string& str ){
	vector<string> string_items;
	stringstream stream( str );
	string single_item;

	while( stream >> single_item ) 
	    string_items.push_back( single_item );

	return string_items;
    }

    /* Parse a list of space seperated integers, and return as a vector */
    vector<int> split_integers( const string& str ){
	vector<int> integer_items;
	stringstream stream( str );
	int single_item;

	while( stream >> single_item ) 
	    integer_items.push_back( single_item );
	return integer_items;
    }


    void convert_lower( char& c ){
	if( c >= 'A' && c <= 'Z' ) c = c - 'A' + 'a';
    }

    void convert_lower( string& str ){
	EACH(it,str)
	    convert_lower( *it );
    }



}
















