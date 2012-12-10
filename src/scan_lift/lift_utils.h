//#include "myheader.h"
#ifndef LIFT_UTILS_H
#define LIFT_UTILS_H
namespace Utils {
  
  using namespace std;
  
  bool is_space( const char& c );
  vector<string> split_strings( const string& str );
  vector<int> split_integers( const string& str );
  
  void convert_lower( char& c );
  void convert_lower( string& str );
  
}
#endif
