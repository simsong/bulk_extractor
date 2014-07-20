/**
 *
 * ABOUT:
 *	A standalone program debug scanner. Compile in the scanner you want.
 *
 *
 */

#include "bulk_extractor.h"
#include "findopts.h"
#include "image_process.h"
#include "threadpool.h"
#include "be13_api/aftimer.h"
#include "be13_api/histogram.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"
#include "be13_api/unicode_escape.h"

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

void usage()
{
    std::cerr << "usage: stand [options] filename\n";
    std::cerr << "Options:\n";
    std::cerr << "   -h           - print this message\n";
    std::cerr << "  -e scanner    - enable scanner\n";
    std::cerr << "  -o outdir     - specify output directory\n";
    be13::plugin::info_scanners(false,true,scanners_builtin,'e','x');
}

int main(int argc,char **argv)
{

    scanner_info::scanner_config   s_config; 
    be13::plugin::load_scanners(scanners_builtin,s_config); 

    /* look for usage first */
    if(argc==1 || (strcmp(argv[1],"-h")==0)){
	usage();
	return(1);
    }
    
    int ch;
    std::string opt_outdir;
    while ((ch = getopt(argc, argv, "e:o:s:x:h?")) != -1) {
	switch (ch) {
	case 'o': opt_outdir = optarg;break;
	case 'e': be13::plugin::scanners_enable(optarg);break;
	case 'x': be13::plugin::scanners_disable(optarg);break;
	case 's':
	    {
		std::vector<std::string> params = split(optarg,'=');
		if(params.size()!=2){
		    std::cerr << "Invalid paramter: " << optarg << "\n";
		    exit(1);
		}
		//be_config[params[0]] = params[1];
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

    //opt_scan_bulk_block_size = stoi64(be_config["bulk_block_size"]);

    be13::plugin::scanners_process_enable_disable_commands();

    feature_file_names_t feature_file_names;
    be13::plugin::get_scanner_feature_file_names(feature_file_names);
    feature_recorder_set fs(0);	// where the features will be put
    fs.init(feature_file_names,argv[0],opt_outdir);
    be13::plugin::scanners_init(fs);

    /* Make the sbuf */
    sbuf_t *sbuf = sbuf_t::map_file(argv[0]);
    if(!sbuf){
	err(1,"Cannot map file %s:%s\n",argv[0],strerror(errno));
    }

    be13::plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,*sbuf,fs));
    be13::plugin::phase_shutdown(fs);
    fs.process_histograms(0);
    return(0);
}

