/**
 * Plugin: scan_lift
 * Purpose: The purpose of this plugin is to scan and type identify buffers presented
 *  to the pretrained scanner.
 * 
 * The algorithm works as follows:
 * - buffer is scanned by the type identifier
 * - if a match is presented the following actions will take place
 * - create a text file in the type_id directory specified by the user 
 * - the file contains the following information:
 * -- buffer string, type identified

scan_lift:

LIFT is CMU's "Learning to Identify File Types," a file fragment type
identification system developed by Carnegie Mellon Univeristy and
substantially modified by NPS. 

LIFT is run through the scan_lift scanner:

  ./bulk_extractor -E lift -s liftcmd=help	- prints help
  ./bulk_extractor -E lift -s liftcmd=a2c -s if=<model-dir> of=<filename.cpp>


 **/

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <sstream>

using namespace std;
std::string opt_model_dir("scan_lift/model");

#include "scan_lift/lift_svm.h"
/* These two variables allow access to the SVM from any scanner, even when it is called recursively.
 * scope after initialization and when the scanner enters scanner_params::scan
 */
lift_svm classifier;

static void liftcmd_usage()  __attribute__((__noreturn__));
static void liftcmd_usage() 
{
    std::cout << "usage: bulk_extractor -E lift -s liftcmd=<command>  [-s liftif=<modelfile>] [-s liftof=<modelfile>]\n";
    std::cout << " commands:\n";
    std::cout << "   help - print this message\n";
    std::cout << "   info - print info about loaded scanner\n";
    std::cout << "   a2c - convert ascii to C++\n";
    exit(1);
}

static void liftcmd_info()  __attribute__((__noreturn__));
void liftcmd_info()
{
    std::cout << "   info - prints info about the lift model file\n";
    classifier.load_model_dir(opt_model_dir);
    classifier.dump(std::cout);		// debug
    exit(1);
}

static std::string liftif;
static std::string liftof;
static void liftcmd_a2c() __attribute__((__noreturn__));
static void liftcmd_a2c()
{
    if(liftif.size()==0) err(1,"please specify -s liftif=<filename>");
    if(liftof.size()==0) err(1,"please specify -s liftof=<filename>");
    classifier.load_model_dir(liftif);
    classifier.save_cpp(liftof);
    exit(1);
}

/**
 * void scan_lift.cpp
 * 
 * 
 **/
static size_t opt_bulk_block_size = 512;        //                                                                                                               
extern "C"
void scan_lift(const class scanner_params &sp, const recursion_control_block &rcb){
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name 		= "lift";
	sp.info->author		= "Tony Melaragno and Simson Garfinkel";
	sp.info->description	= "performs SVM recognition of file fragment types";
	sp.info->feature_names.insert("lift_tags");

	sp.info->flags		= (scanner_info::SCANNER_DISABLED
				   |scanner_info::SCANNER_NO_USAGE
				   |scanner_info::SCANNER_NO_ALL);
   
        std::string liftcmd;
        sp.info->get_config("liftcmd",&liftcmd,"LIFT Command");
        sp.info->get_config("liftif",&liftif,"LIFT input file");
        sp.info->get_config("liftof",&liftof,"LIFT output file");
        sp.info->get_config("bulk_block_size",&opt_bulk_block_size,"Block size (in bytes) for bulk data analysis");

        if(liftcmd!=""){
            if(liftcmd=="help"){
                liftcmd_usage();
            }
            if(liftcmd=="info"){
                liftcmd_info();
            }
            if(liftcmd=="a2c"){
                liftcmd_a2c();
            }
            liftcmd_usage();
	    exit(1);
	}

	/**
	 * Load a model if a file was provided.
	 * Otherwise use the built-in model.
	 */
	if(liftif!=""){
	    classifier.load_model_dir(liftif);
	} else {
	    classifier.load_pretrained();
	}
	return;
    }

    // classify 
    if(sp.phase==scanner_params::PHASE_SCAN){

	feature_recorder *lift_tags = sp.fs.get_name("lift_tags");


	// Loop through the sbuf in opt_bulk_block_size sized chunks
	// for each one, examine the entropy and scan for bitlocker (unfortunately hardcoded)
	// This needs to have a general plug-in architecture
	for(size_t base=0;base+opt_bulk_block_size<=sp.sbuf.pagesize;base+=opt_bulk_block_size){
	    sbuf_t sbuf(sp.sbuf,base,opt_bulk_block_size);

	    double theScore=0;
	    string result = classifier.classify(sbuf,&theScore);
	
	    if(result.length() > 0 ) {
		// if a result is generated then at this point we pass it to the feature recorder
		std::stringstream ss;
		ss << result << " {lift_score=" << theScore << "}";
		lift_tags->write_tag(sbuf.pos0, sbuf.pagesize, ss.str());
	    }
	}
	return;
    }

    // shutdown --- this will be handled by scan_bulk.cpp
}
