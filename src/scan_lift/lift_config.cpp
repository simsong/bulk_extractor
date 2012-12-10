#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lift_config.h"
#include <cassert>
namespace Config {
    //using namespace std;
    static Configurations C;
    void Configurations::parse( const string& filename ){
	FILE *f = fopen( filename.c_str() , "rb" );
	assert( f );
	static char buff[1<<10];
	while(fgets( buff , 1023 , f )){
	    string key, value;
	    int i = 0, j;
	    for( j = 0; buff[ j ] && buff[ j ] == ' '; j++ );
	    if( buff[ j ] == '#' || strlen( buff ) <= 2 ) continue;
	    for(i = 0; buff[ i ] && buff[ i ] != '='; ++i ) key += buff[ i ];
	    if( buff[ i ] != '=' ) {
		puts(" Wrong format \n" );exit(0);
	    }
	    while( buff[ ++i ] != '\n' && buff[ i ] ) value += buff[ i ];
	    /* Remove space */
	    while( key.size() && *(key.begin()) == ' ' ) key.erase( key.begin() );
	    while( value.size() && *(value.begin()) == ' ' ) value.erase( value.begin() );
	    while( key.size() && *(--key.end()) == ' ' ) key.erase( --key.end() );
	    while( value.size() && *(--value.end()) == ' ' ) value.erase( --value.end() );
	    kv_pairs[ key ] = value;
	}
	// Perform variable substitution, ie if b = sdffds ; a = $b , then set b = sdffds

	for( map<string,string>::iterator it = kv_pairs.begin() ; it != kv_pairs.end() ; ++it ) {
	    if( it->second.size() && it->second[0] == '$' ) {
		it->second.erase( it->second.begin() );
		it->second = kv_pairs[ it->second ];
	    }
	}
	fclose( f );
    }
    string Configurations::get_value( const string& key ){
	if( kv_pairs.count( key ) ) return kv_pairs[ key ];
	return "";
    }
    void Configurations::set( const string& key , const string& value ){
	kv_pairs[ key ] = value;
    }
    string get_value( const string& key ){
	return C.get_value( key );
    }
    void init( const string& filename ){
        C.parse( filename );
    }
}

