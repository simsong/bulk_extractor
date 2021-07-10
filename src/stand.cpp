/**
 *
 * ABOUT:
 *	A standalone program debug scanner.
 *      Solely for testing.
 *      Compile in the scanner you want.
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
//#include <errno.h>
#include <sstream>
#include <vector>

#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"

/**
 * Stand alone tester.
 * 1. allocate an sbuf for the file.
 * 2. Initialize the scanner.
 * 3. Call the scanner.
 * 4. Shut down the scanner.
 */
int debug=0;
const char *image_fname = 0;

/* Bring in the definitions for the  */
#define SCANNER(scanner) extern "C" scanner_t scan_ ## scanner;
#include "bulk_extractor_scanners.h"
#undef SCANNER

scanner_t *scanners_builtin[] = {
    scan_json,                          // the only built-in scanner for now
    0
};

void usage(scanner_set &ss)
{
    std::cerr << "usage: stand [options] filename\n";
    std::cerr << "Options:\n";
    std::cerr << "   -h           - print this message\n";
    std::cerr << "  -e scanner    - enable scanner\n";
    std::cerr << "  -o outdir     - specify output directory\n";
    ss.info_scanners(std::cerr, false, true, 'e','x');
}

int main(int argc,char **argv)
{
    scanner_config   sc;
    struct feature_recorder_set::flags_t f;
    sc.outdir =  std::filesystem::temp_directory_path().string(); // default

    /* look for usage first */
    if(argc==1 || (strcmp(argv[1],"-h")==0)){
        scanner_set ss(sc, f);    // great a bogus scanner_set for help info
	usage(ss);
	return(1);
    }

    int ch;
    while ((ch = getopt(argc, argv, "e:o:s:x:h?")) != -1) {
	switch (ch) {
	case 'o': sc.outdir = optarg;break;
	case 'e': sc.push_scanner_command(optarg, scanner_config::scanner_command::ENABLE);break;
	case 'x': sc.push_scanner_command(optarg, scanner_config::scanner_command::DISABLE); break;
	case 's':
	    {
		std::vector<std::string> params = split(optarg,'=');
		if(params.size()!=2){
		    std::cerr << "Invalid paramter: " << optarg << "\n";
		    exit(1);
		}
		sc.namevals[params[0]] = params[1];
		continue;
	    }
	case 'h': case '?':default:
            {
                scanner_set ss(sc, f);    // great a bogus scanner_set for help info
                usage(ss);
                exit(1);
            }
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    scanner_set ss(sc, f);
    if(argc!=1) usage(ss);

    ss.apply_scanner_commands();        // applied after all scanners are added


    /* Make an sbuf for the file. */
    sbuf_t *sbuf = sbuf_t::map_file(argv[0]);
    ss.phase_scan();                    // start the scanner phase
    ss.process_sbuf( sbuf );
    ss.shutdown();
    return(0);
}
