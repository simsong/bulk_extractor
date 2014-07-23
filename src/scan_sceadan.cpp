/**
 * Plugin: scan_sceadan
 * Purpose: The purpose of this plugin is to scan and type identify buffers presented
 *  to the pretrained scanner.
 * 
 * The algorithm works as follows:
 * - buffer is scanned by the type identifier
 * - if a match is presented the following actions will take place
 * - create a text file in the type_id directory specified by the user 
 * - the file contains the following information:
 * -- buffer string, type identified

SCEADAN is run through the scan_sceadan scanner:

  ./bulk_extractor -E sceadan -s scandan_model=model_file   - model_file is optional


 **/

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <sstream>


/**
 * void scan_sceadan.cpp
 * 
 * 
 **/
#ifdef USE_SCEADAN
#include "sceadan.h"
static size_t opt_sceadan_block_size = 16384;
static std::string sceadan_model_file; 
static std::string sceadan_class_file; 
static std::string sceadan_mask_file; 
static sceadan *s = 0;
static int sceadan_mode = 0;
static int sceadan_ngram_mode = 2;
#endif
                 
extern "C"
void scan_sceadan(const class scanner_params &sp, const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name 		= "sceadan";
	sp.info->author		= "Simson Garfinkel";
	sp.info->flags		= (scanner_info::SCANNER_DISABLED
                                   |scanner_info::SCANNER_WANTS_NGRAMS
                                   |scanner_info::SCANNER_DEPTH_0
				   |scanner_info::SCANNER_NO_ALL);
   
#ifdef USE_SCEADAN
	sp.info->description	= "performs statistical file type recognition of file fragment types";
	sp.info->feature_names.insert("sceadan");

        sp.info->histogram_defs.insert(histogram_def("sceadan","","histogram"));
        std::string liftcmd;
        sp.info->get_config("sceadan_mode",&sceadan_mode,"0=memory histogram only; 1 = report all classified blocks");
        sp.info->get_config("sceadan_model_file",&sceadan_model_file,"Model file to use");
        sp.info->get_config("sceadan_class_file",&sceadan_class_file,"Class file to use");
        sp.info->get_config("sceadan_mask_file",&sceadan_mask_file,"Mask file to use");
        sp.info->get_config("sceadan_block_size",&opt_sceadan_block_size,"Block size (in bytes) for bulk data analysis");
#else
	sp.info->description	= "DISABLED";
#endif
    }

    if(sp.phase==scanner_params::PHASE_INIT){
#ifdef USE_SCEADAN
        s = sceadan_open(sceadan_model_file.c_str(),sceadan_class_file.c_str(),sceadan_mask_file.c_str());
        if(!s){
            std::cerr << "Cannot open sceadan classifier\n";
            exit(1);
        }
        sceadan_set_ngram_mode(s,sceadan_ngram_mode);
        feature_recorder *sceadanfs = sp.fs.get_name("sceadan");
        sceadanfs->set_flag(feature_recorder::FLAG_NO_FEATURES);
        sceadanfs->enable_memory_histograms(); // is this really how they are enabled?
#else
        std::cerr << "scan_sceadan: not compiled in, will not enable.\n";
#endif
    }


#ifdef USE_SCEADAN
    // classify 
    if(sp.phase==scanner_params::PHASE_SCAN){

	feature_recorder *sceadanfs = sp.fs.get_name("sceadan");


	// Loop through the sbuf in opt_bulk_block_size sized chunks
	// for each one, examine the entropy and scan for bitlocker (unfortunately hardcoded)
	// This needs to have a general plug-in architecture
	for(size_t base=0 ; base+opt_sceadan_block_size <= sp.sbuf.pagesize ;
            base+=opt_sceadan_block_size){
	    sbuf_t sbuf (sp.sbuf, base, opt_sceadan_block_size );

            int code = sceadan_classify_buf(s,sbuf.buf, sbuf.bufsize);
            const char *type = sceadan_name_for_type(s,code);
            if(type) {
                sceadanfs->write(sbuf.pos0,type,"");
            } else {
                char buf[20];
                snprintf(buf,sizeof(buf),"SCEADAN%d",code);
                sceadanfs->write(sbuf.pos0,buf,"");
            }
	}
    }
    // shutdown --- this will be handled by scan_bulk.cpp
#endif
}
