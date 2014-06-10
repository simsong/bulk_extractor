// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Generates MD5 hash values from hashdb_block_size data taken along sector
 * boundaries and scans for matches against a hash database.
 *
 * Note that the hash database may be accessed locally through the
 * file system or remotely through a socket.
 */

#include "config.h"
#include "bulk_extractor.h"

#ifdef HAVE_HASHID

#include "hashdb.hpp"
#include <dfxml/src/hash_t.h>

#include <iostream>
#include <unistd.h>	// for getpid
#include <sys/types.h>	// for getpid

// user settings
static std::string hashdb_mode="none";                       // import or scan
static uint32_t hashdb_block_size=4096;                      // import or scan
static std::string hashdb_path_or_socket="your_hashdb_directory"; // scan only
static size_t hashdb_sector_size = 512;                         // scan only
static std::string hashdb_repository_name="default_repository"; // import only
static uint32_t hashdb_max_duplicates=0;                        // import only

// runtime modes
// scanner mode
enum mode_type_t {MODE_NONE, MODE_SCAN, MODE_IMPORT};
static mode_type_t mode = MODE_NONE;

// internal helper functions
static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb);
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb);

// global state

// hashdb directory, import only
static std::string hashdb_dir;

// hashdb manager
typedef hashdb_t__<md5_t> hashdb_t;
hashdb_t* hashdb;

extern "C"
void scan_hashid(const class scanner_params &sp,
                 const recursion_control_block &rcb) {

    switch(sp.phase) {
        // startup
        case scanner_params::PHASE_STARTUP: {

            // set properties for this scanner
            sp.info->name        = "hashid";
            sp.info->author      = "Bruce Allen";
            sp.info->description = "Search cryptographic hash IDs against hashes in a hashdb block hash database";
            sp.info->flags       = scanner_info::SCANNER_DISABLED;

            // hashdb_mode
            std::stringstream ss_hashdb_mode;
            ss_hashdb_mode << "Operational mode [none|import|scan]\n"
                << "        none    - The scanner is active but performs no action.\n"
                << "        import  - Import block hashes.\n"
                << "        scan    - Scan for matching block hashes.";
            sp.info->get_config("hashdb_mode", &hashdb_mode, ss_hashdb_mode.str());

            // hashdb_block_size
            sp.info->get_config("hashdb_block_size", &hashdb_block_size,
                         "Hash block size, in bytes, used to generte hashes");

            // hashdb_path_or_socket
            std::stringstream ss_hashdb_path_or_socket;
            ss_hashdb_path_or_socket
                << "File path to a hash database or\n"
                << "      socket to a hashdb server to scan against.  Valid only in scan mode.";
            sp.info->get_config("hashdb_path_or_socket", &hashdb_path_or_socket,
                                ss_hashdb_path_or_socket.str());

            // hashdb_sector_size
            std::stringstream ss_hashdb_sector_size;
            ss_hashdb_sector_size
                << "Selects the sector size.  Scans along sector boundaries.\n"
                << "      Valid only in scan mode.";
            sp.info->get_config("hashdb_sector_size", &hashdb_sector_size,
                                ss_hashdb_sector_size.str());

            // hashdb_repository_name
            std::stringstream ss_hashdb_repository_name;
            ss_hashdb_repository_name
                << "Sets the repository name to attribute\n"
                << "      the import to.  Valid only in import mode.";
            sp.info->get_config("hashdb_repository_name",
                                &hashdb_repository_name,
                                ss_hashdb_repository_name.str());

            // hashdb_max_duplicates
            std::stringstream ss_hashdb_max_duplicates;
            ss_hashdb_max_duplicates
                << "The maximum number of duplicates to import\n"
                << "      for a given hash value, or 0 for no limit.  Valid only in import mode.";
            sp.info->get_config("hashdb_max_duplicates", &hashdb_max_duplicates,
                                ss_hashdb_max_duplicates.str());


            // configure the feature file to accept scan features
            // but only if in scan mode
            if (hashdb_mode == "scan") {
                sp.info->feature_names.insert("identified_blocks");
            }

            return;
        }

        // init
        case scanner_params::PHASE_INIT: {
            // validate the input parameters

            // hashdb_mode
            if (hashdb_mode == "none") {
                mode = MODE_NONE;
            } else if (hashdb_mode == "import") {
                mode = MODE_IMPORT;
            } else if (hashdb_mode == "scan") {
                mode = MODE_SCAN;
            } else {
                // bad mode
                std::cerr << "Error.  Parameter 'hashdb_mode' value '"
                          << hashdb_mode << "' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_block_size
            if (hashdb_block_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_block_size' is invalid.\n"
                         << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_path_or_socket
            // checks not performed

            // hashdb_sector_size
            if (hashdb_sector_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_sector_size' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // also, for valid operation, sectors must align on hash block boundaries
            if (hashdb_block_size % hashdb_sector_size != 0) {
                std::cerr << "Error: invalid hashdb block size=" << hashdb_block_size
                          << " or hashdb sector size=" << hashdb_sector_size << ".\n"
                          << "Sectors must align on hash block boundaries.\n"
                          << "Specifically, hashdb_block_size \% hashdb_sector_size must be zero.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // perform setup based on mode
            switch(mode) {
                case MODE_IMPORT: {
                    // set the path to the hashdb
                    hashdb_dir = sp.fs.get_outdir() + "/" + "hashdb.hdb";

                    // create the new hashdb manager for importing
                    // currently, hashdb_dir is required to not exist
                    hashdb = new hashdb_t(hashdb_dir,
                                          hashdb_block_size,
                                          hashdb_max_duplicates);

                    // show relavent settable options
                    std::cout << "hashid: hashdb_mode=" << hashdb_mode << "\n"
                              << "hashid: hashdb_block_size=" << hashdb_block_size << "\n"
                              << "hashid: hashdb_max_duplicates=" << hashdb_block_size << "\n";
                    std::cout << "hashid: Creating hashdb directory " << hashdb_dir << "\n";
                    return;
                }

                case MODE_SCAN: {
                    // show relavent settable options
                    std::cout << "hashid: hashdb_mode=" << hashdb_mode << "\n"
                              << "hashid: hashdb_block_size=" << hashdb_block_size << "\n"
                              << "hashid: hashdb_path_or_socket=" << hashdb_path_or_socket << "\n"
                              << "hashid: hashdb_sector_size=" << hashdb_sector_size << "\n";

                    // open the hashdb manager for scanning
                    hashdb = new hashdb_t(hashdb_path_or_socket);
                    return;
                }

                case MODE_NONE: {
                    // show relavent settable options
                    std::cout << "hashid: hashdb_mode=" << hashdb_mode << "\n"
                              << "WARNING: the hashid scanner is enabled but it will not perform any action\n"
                              << "because no mode has been selected.  Please either select a hashdb mode or\n"
                              << "leave the hashid scanner disabled to avoid this warning.\n";

                    // no action
                    return;
                }
                    
                default: {
                    // program error
                    assert(0);
                }
            }
        }

        // scan
        case scanner_params::PHASE_SCAN: {
            switch(mode) {
                case MODE_IMPORT:
                     do_import(sp, rcb);
                     return;

                case MODE_SCAN:
                     do_scan(sp, rcb);
                     return;
                default:
                     // the user should have just left the scanner disabled.
                     // no action.
                     return;
            }
        }

        // shutdown
        case scanner_params::PHASE_SHUTDOWN: {
            switch(mode) {
                case MODE_SCAN:
                     delete hashdb;
                     return;
                case MODE_IMPORT:
                     delete hashdb;
                     return;
                default:
                     return;
            }
        }

        // there are no other bulk_extractor scanner state actions
        default: {
            // no action for other bulk_extractor scanner states
            return;
        }
    }
}

// perform import
static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb) {

    // allocate space on heap for import_input
    std::vector<hashdb_t::import_element_t>* import_input =
                           new std::vector<hashdb_t::import_element_t>;

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // get all the cryptograph hash values of all the blocks from sbuf
    for (size_t i=0; i + hashdb_block_size <= sbuf.pagesize; i += hashdb_block_size) {
        // calculate the hash for this sector-aligned hash block
        md5_t hash = md5_generator::hash_buf(sbuf.buf + i, hashdb_block_size);

        // create the import element
        hashdb_t::import_element_t import_element(hash,
                                           hashdb_repository_name,
                                           sbuf.pos0.str(), // use as filename
                                           i);              // file offset
        // add the hash to the import input
        import_input->push_back(import_element);
    }

    // perform the import
    int status = hashdb->import(*import_input);

    if (status != 0) {
        std::cerr << "scan_hashid import failure\n";
    }

    // clean up
    delete import_input;
}

// perform scan
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb) {

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // make sure there is enough data to calculate at least one cryptographic hash
    if (sbuf.pagesize < hashdb_block_size) {
      // not enough data
      return;
    }

    // number of hashes is highest index + 1
    size_t num_hashes = ((sbuf.pagesize - hashdb_block_size) / hashdb_sector_size) + 1;

    // allocate space on heap for scan_input
    std::vector<md5_t>* scan_input = new std::vector<md5_t>(num_hashes);

    // get all the cryptograph hash values of all the blocks along
    // sector boundaries from sbuf
    for (size_t i=0; i< num_hashes; ++i) {
        // calculate the hash for this sector-aligned hash block
        md5_t hash = md5_generator::hash_buf(sbuf.buf + i*hashdb_sector_size, hashdb_block_size);

        // add the hash to scan input
        (*scan_input)[i] = hash;
    }

    // allocate space on heap for scan_output
    hashdb_t::scan_output_t* scan_output = new hashdb_t::scan_output_t;

    // perform the scan
    int status = hashdb->scan(*scan_input, *scan_output);

    if (status != 0) {
        std::cerr << "Error: scan_hashid scan failure.  Aborting.\n";
        exit(1);
    }

    // get the feature recorder
    feature_recorder* identified_blocks_recorder = sp.fs.get_name("identified_blocks");

    // record each feature returned in the response
    for (hashdb_t::scan_output_t::const_iterator it=scan_output->begin(); it!= scan_output->end(); ++it) {

        // prepare forensic path (pos0, feature, context) as (pos0, hash_string, count_string)

        // pos0
        pos0_t pos0 = sbuf.pos0 + it->first * hashdb_sector_size;

        // hash_string
        std::string hash_string = (*(scan_input))[it->first].hexdigest();

        // count
        std::stringstream ss;
        ss << it->second;
        std::string count_string = ss.str();

        // record the feature
        identified_blocks_recorder->write(pos0, hash_string, count_string);
    }

    // clean up
    delete scan_input;
    delete scan_output;
}

#endif

