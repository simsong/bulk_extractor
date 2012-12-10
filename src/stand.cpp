/**
 *
 * ABOUT:
 *	A standalone program debug scanner. Compile in the scanner you want.
 *
 *
 */

#include "bulk_extractor.h"

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
be_config_t be_config; // system configuration

scanner_t *scanners_builtin[] = {
    scan_bulk,
    0
};

void usage()
{
    load_scanners(scanners_builtin,histograms);
    cerr << "usage: stand [options] filename\n";
    cerr << "Scanner(s):\n";
    for(scanner_vector::const_iterator it = current_scanners.begin();it!=current_scanners.end();it++){
	cerr << "   " << (*it)->info.name;
	if((*it)->info.flags & scanner_info::SCANNER_NO_USAGE){ cerr << " SCANNER_NO_USAGE "; }
	if((*it)->info.flags & scanner_info::SCANNER_DISABLED){ cerr << " SCANNER_DISABLED "; }
	cerr << "\n";
    }
    
    cerr << "Options:\n";
    cerr << "   -h           - print this message\n";
    cerr << "  -e scanner    - enable scanner\n";
    cerr << "  -o outdir     - specify output directory\n";
    exit(1);
}

int main(int argc,char **argv)
{
    /* look for usage first */
    if(argc==1 || (strcmp(argv[1],"-h")==0)){
	usage();
	exit(1);
    }
    load_scanners(scanners_builtin,histograms);
    be_config["bulk_block_size"] = itos(opt_scan_bulk_block_size);
    
    int ch;
    std::string opt_outdir;
    while ((ch = getopt(argc, argv, "e:o:s:x:h?")) != -1) {
	switch (ch) {
	case 'o': opt_outdir = optarg;break;
	case 'e': set_scanner_enabled(optarg,1);break;
	case 'x': set_scanner_enabled(optarg,0);break;
	case 's':
	    {
		std::vector<std::string> params = split(optarg,'=');
		if(params.size()!=2){
		    std::cerr << "Invalid paramter: " << optarg << "\n";
		    exit(1);
		}
		be_config[params[0]] = params[1];
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

    opt_scan_bulk_block_size = stoi(be_config["bulk_block_size"]);

    feature_file_names_t feature_file_names;
    enable_feature_recorders(feature_file_names);

    xml *xreport = new xml("stand.xml",false);

    /* Make the sbuf */
    sbuf_t *sbuf = sbuf_t::map_file(argv[0]);
    if(!sbuf){
	err(1,"Cannot map file %s:%s\n",argv[0],strerror(errno));
    }

    feature_recorder_set fs(feature_file_names,opt_outdir);	// where the features will be put
    scanner_params sp(*sbuf,fs);		// the parameters that we will pass the scanner
    recursion_control_block rcb(0,"STAND",true);
    
    for(scanner_vector::iterator it = current_scanners.begin();it!=current_scanners.end();it++){
	((*it)->scanner)(sp,rcb);
    }

    phase2(fs,*xreport);

    cout << "Phase 3. Creating Histograms\n";
    for(histograms_t::const_iterator it = histograms.begin();it!=histograms.end();it++){
	if(fs.has_name((*it).feature)){
	    feature_recorder *fr = fs.get_name((*it).feature);
	    try {
		fr->make_histogram((*it).pattern,(*it).suffix);
	    }
	    catch (const std::exception &e) {
		cerr << "ERROR: " << e.what() << " computing histogram " << (*it).feature << "\n";
		xreport->xmlout("error",(*it).feature, "function='phase3'",true);
	    }
	}
    }

    return(0);
}

