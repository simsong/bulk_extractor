/**
 *
 * ABOUT:
 *	A standalone program debug scanner. Compile in the scanner you want.
 *
 *
 */

#include "config.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sstream>
#include <vector>

#include "scanners.h"
#include "be13_api/scanner_set.h"

//#include "bulk_extractor.h"
//#include "findopts.h"
//#include "image_process.h"
//#include "threadpool.h"
////#include "be13_api/aftimer.h"
//#include "be13_api/histogram.h"
//#include "dfxml/src/dfxml_writer.h"
//#include "dfxml/src/hash_t.h"
//#include "be13_api/unicode_escape.h"


/**
 * Stand alone tester.
 * 1. allocate an sbuf for the file.
 * 2. Initialize the scanner.
 * 3. Call the scanner.
 * 4. Shut down the scanner.
 */
int debug=0;
const char *image_fname = 0;
//be_config_t be_config; // system configuration

scanner_t *scanners_builtin[] = {
    scan_bulk,
    0
};

void usage(scanner_set &ss)
{
    std::cerr << "usage: stand [options] filename\n";
    std::cerr << "Options:\n";
    std::cerr << "   -h           - print this message\n";
    std::cerr << "  -e scanner    - enable scanner\n";
    std::cerr << "  -o outdir     - specify output directory\n";
    ss.info_scanners(std::cerr, false,true, 'e','x');
}

int main(int argc,char **argv)
{
    scanner_config   sc;
    struct feature_recorder_set::flags_t f;
    //be13::plugin::load_scanners(scanners_builtin,sc);

    /* look for usage first */
    if(argc==1 || (strcmp(argv[1],"-h")==0)){
        scanner_set ss(sc, f);    // great a bogus scanner_set for help info
	usage(ss);
	return(1);
    }

    int ch;
    std::string opt_outdir;
    while ((ch = getopt(argc, argv, "e:o:s:x:h?")) != -1) {
	switch (ch) {
	case 'o': opt_outdir = optarg;break;
	case 'e': sc.push_scanner_command(optarg, true);break;
	case 'x': sc.push_scanner_command(optarg, false); break;
	case 's':
	    {
		std::vector<std::string> params = split(optarg,'=');
		if(params.size()!=2){
		    std::cerr << "Invalid paramter: " << optarg << "\n";
		    exit(1);
		}
		c_config.namevals[params[0]] = params[1];
		continue;
	    }
	case 'h': case '?':default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if(argc!=1) usage();
    scanner_set ss(sc, f);

    //opt_scan_bulk_block_size = stoi64(be_config["bulk_block_size"]);

    //be13::plugin::scanners_process_enable_disable_commands();

    //feature_file_names_t feature_file_names;
    //be13::plugin::get_scanner_feature_file_names(feature_file_names);
    //feature_recorder_set fs(0);	// where the features will be put
    //fs.init(feature_file_names,argv[0],opt_outdir);
    //be13::plugin::scanners_init(fs);

    /* Make an sbuf for the file. */
    sbuf_t sbuf = sbuf_t::map_file(argv[0]);
    if(!sbuf){
	err(1,"Cannot map file %s:%s\n",argv[0],strerror(errno));
    }

    ss.process_sbuf( sbuf );
    ss.shutdown();
    return(0);
}
