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

#include "bulk_extractor.h"

#ifdef HAVE_HASHID

#include "hashdb.hpp"
#include <dfxml/src/hash_t.h>

#include <iostream>
#include <unistd.h>	// for getpid
#include <sys/types.h>	// for getpid

// user settings
static std::string hashdb_mode="none";
static std::string hashdb_hashdigest_type="MD5";
static uint32_t hashdb_block_size=4096;
static uint32_t hashdb_max_duplicates=20;
static std::string hashdb_path_or_socket="your_hashdb_directory";
static size_t hashdb_sector_size = 512;

// runtime modes
// scanner mode
enum mode_type_t {MODE_NONE, MODE_SCAN, MODE_IMPORT};
static mode_type_t mode = MODE_NONE;

// hashdigest type
enum hashdigest_type_t {HASHDIGEST_MD5, HASHDIGEST_SHA1, HASHDIGEST_SHA256};
static hashdigest_type_t hashdigest_type = HASHDIGEST_MD5;

// internal helper functions
template<typename T, typename T_GEN>
static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb);
template<typename T, typename T_GEN>
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb);

// global state

// hashdb directory, import only
static std::string hashdb_dir;

// hashdb manager
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

            // hashdb_mode
            std::stringstream ss_hashdb_mode;
            ss_hashdb_mode << "Operational mode [none|import|scan]\n"
                << "        none    - The scanner is active but performs no action.\n"
                << "        import  - Import block hashes.\n"
                << "        scan    - Scan for matching block hashes.";
            sp.info->get_config("hashdb_mode", &hashdb_mode, ss_hashdb_mode.str());

            // hashdb_hashdigest_type
            std::stringstream ss_hashdb_hashdigest_type;
            ss_hashdb_hashdigest_type << "Cryptographic hash algorithm to use,\n"
                << "      one of [MD5|SHA1|SHA256].";
            sp.info->get_config("hashdb_hashdigest_type",
                                &hashdb_hashdigest_type,
                                ss_hashdb_hashdigest_type.str());

            // hashdb_block_size
            sp.info->get_config("hashdb_block_size", &hashdb_block_size,
                                "Block size, in bytes, used to generate hashes");

            // hashdb_max_duplicates
            std::stringstream ss_hashdb_max_duplicates;
            ss_hashdb_max_duplicates
                << "The maximum number of duplicates to import\n"
                << "      for a given hash value.  Valid only in import mode.";
            sp.info->get_config("hashdb_max_duplicates", &hashdb_max_duplicates,
                                ss_hashdb_max_duplicates.str());

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

            // hashdb_hashdigest_type
            if (hashdb_hashdigest_type == "MD5") {
                hashdigest_type = HASHDIGEST_MD5;
            } else if (hashdb_hashdigest_type == "SHA1") {
                hashdigest_type = HASHDIGEST_SHA1;
            } else if (hashdb_hashdigest_type == "SHA256") {
                hashdigest_type = HASHDIGEST_SHA256;
            } else {
                // bad mode
                std::cerr << "Error.  Parameter 'hashdb_hashdigest_type' value '"
                          << hashdb_hashdigest_type << "' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_block_size
            if (hashdb_block_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_block_size' is invalid.\n"
                         << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_max_duplicates
            if (hashdb_max_duplicates == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_max_duplicates' is invalid.\n"
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
                    hashdb_dir = sp.fs.get_outdir() + "/" + "hashdb";

                    // create the new hashdb manager for importing
                    // currently, hashdb_dir is required to not exist
                    hashdb = new hashdb_t(hashdb_dir,
                                                  hashdb_hashdigest_type,
                                                  hashdb_block_size,
                                                  hashdb_max_duplicates);
                    return;
                }

                case MODE_SCAN: {
                    // open the hashdb manager for scanning
                    hashdb = new hashdb_t(hashdb_path_or_socket);
                    return;
                }

                case MODE_NONE: {
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
                     switch(hashdigest_type) {
                         case HASHDIGEST_MD5:
                             do_import<md5_t, md5_generator>(sp, rcb);
                             return;
                         case HASHDIGEST_SHA1:
                             do_import<sha1_t, sha1_generator>(sp, rcb);
                             return;
                         case HASHDIGEST_SHA256:
                             do_import<sha256_t, sha256_generator>(sp, rcb);
                             return;
                         default: {
                             // program error
                             assert(0);
                         }
                     }

                case MODE_SCAN:
                     switch(hashdigest_type) {
                         case HASHDIGEST_MD5:
                             do_scan<md5_t, md5_generator>(sp, rcb);
                             return;
                         case HASHDIGEST_SHA1:
                             do_scan<sha1_t, sha1_generator>(sp, rcb);
                             return;
                         case HASHDIGEST_SHA256:
                             do_scan<sha256_t, sha256_generator>(sp, rcb);
                             return;
                         default: {
                             // program error
                             assert(0);
                         }
                     }
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
template<typename T, typename T_GEN>
static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb) {

    // allocate space on heap for import_input
    std::vector<hashdb_t::import_element_t<T> >* import_input =
                           new std::vector<hashdb_t::import_element_t<T> >;

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // get all the cryptograph hash values of all the blocks from sbuf
    for (size_t i=0; i + hashdb_block_size <= sbuf.pagesize; i += hashdb_block_size) {
        // calculate the hash for this sector-aligned hash block
        T hash = T_GEN::hash_buf(sbuf.buf + i, hashdb_block_size);

        // create the import element
        hashdb_t::import_element_t<T> import_element(hash,
                                           "no repository name",
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
template<typename T, typename T_GEN>
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb) {

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // allocate space on heap for scan_input
    std::vector<std::pair<uint64_t, T> >* scan_input = new 
                               std::vector<std::pair<uint64_t, T> >;

    // allocate space on heap for index, hash lookup map
    std::map<uint64_t, T>* hash_lookup = new std::map<uint64_t, T>;

    // get all the cryptograph hash values of all the blocks along
    // sector boundaries from sbuf
    for (size_t i=0; i + hashdb_block_size <= sbuf.pagesize; i += hashdb_sector_size) {
        // calculate the hash for this sector-aligned hash block
        T hash = T_GEN::hash_buf(sbuf.buf + i, hashdb_block_size);

        // add the indexed hash to the scan input
        scan_input->push_back(std::pair<uint64_t, T>(i, hash));

        // add the entry to the hash lookup for later reference
        // zz merge these.
        hash_lookup->insert(std::pair<uint64_t, T>(i, hash));
    }

    // allocate space on heap for scan_output
    hashdb_t::scan_output_t* scan_output = new hashdb_t::scan_output_t;

    // perform the scan
    int status = hashdb->scan(*scan_input, *scan_output);

    if (status != 0) {
        std::cerr << "scan_hashid scan failure\n";
    }

    // get the feature recorder
    feature_recorder* identified_blocks_recorder = sp.fs.get_name("identified_blocks");

    // record each feature returned in the response
    for (hashdb_t::scan_output_t::const_iterator it=scan_output->begin(); it!= scan_output->end(); ++it) {

        // prepare the forensic path
        pos0_t pos0 = sbuf.pos0 + it->first;

        // get hash as string
        uint64_t id = it->first;
        T hash = (*hash_lookup)[id];
        std::string hash_string = hash.hexdigest();

        // get count as string
        std::stringstream ss;
        ss << it->second;
        std::string context = ss.str();

        // record the feature
        identified_blocks_recorder->write(pos0, hash_string, context);
    }

    // clean up
    delete scan_input;
    delete hash_lookup;
    delete scan_output;
}

#endif

