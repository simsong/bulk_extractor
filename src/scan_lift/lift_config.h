#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <fstream>
#include <map>


/* scan_lift configuration object.
 * A single object is created.
 * Originally designed by CMU. To be removed.
 */

namespace Config {

    using namespace std;

    class Configurations {
  
    private:
	map<std::string,std::string> kv_pairs; // config name -> config value

    public:

	Configurations():kv_pairs(){};
	void parse( const std::string& filename );
	std::string get_value( const std::string& key );
	void set( const std::string& key , const std::string& val );

    };

    void init( const std::string& filename ) ;
    std::string get_value( const std::string& key );

}

#endif
