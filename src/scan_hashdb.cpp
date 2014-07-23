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

#ifdef HAVE_HASHDB

#include "hashdb.hpp"
#include <dfxml/src/hash_t.h>

#include <iostream>
#include <unistd.h>	// for getpid
#include <sys/types.h>	// for getpid

// user settings
static std::string hashdb_mode="none";                       // import or scan
static uint32_t hashdb_block_size=4096;                      // import or scan
static bool hashdb_ignore_empty_blocks=true;                 // import or scan
static std::string hashdb_scan_path_or_socket="your_hashdb_directory"; // scan only
static size_t hashdb_scan_sector_size = 512;                    // scan only
static size_t hashdb_import_sector_size = 4096;                 // import only
static std::string hashdb_import_repository_name="default_repository"; // import only
static uint32_t hashdb_import_max_duplicates=0;                 // import only

// runtime modes
// scanner mode
enum mode_type_t {MODE_NONE, MODE_SCAN, MODE_IMPORT};
static mode_type_t mode = MODE_NONE;

// internal helper functions
static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb);
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb);
inline bool is_empty_block(const uint8_t *buf);

// global state

// hashdb directory, import only
static std::string hashdb_dir;

// hash type
typedef md5_t hash_t;
typedef md5_generator hash_generator;

// hashdb manager
typedef hashdb_t__<hash_t> hashdb_t;
hashdb_t* hashdb;

extern "C"
void scan_hashdb(const class scanner_params &sp,
                 const recursion_control_block &rcb) {

    switch(sp.phase) {
        // startup
        case scanner_params::PHASE_STARTUP: {

            // set properties for this scanner
            sp.info->name        = "hashdb";
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
                         "Hash block size, in bytes, used to generate hashes");

            // hashdb_ignore_empty_blocks
            sp.info->get_config("hashdb_ignore_empty_blocks", &hashdb_ignore_empty_blocks,
                         "Selects to ignore empty blocks.");

            // hashdb_scan_path_or_socket
            std::stringstream ss_hashdb_scan_path_or_socket;
            ss_hashdb_scan_path_or_socket
                << "File path to a hash database or\n"
                << "      socket to a hashdb server to scan against.  Valid only in scan mode.";
            sp.info->get_config("hashdb_scan_path_or_socket", &hashdb_scan_path_or_socket,
                                ss_hashdb_scan_path_or_socket.str());

            // hashdb_scan_sector_size
            std::stringstream ss_hashdb_scan_sector_size;
            ss_hashdb_scan_sector_size
                << "Selects the scan sector size.  Scans along\n"
                << "      sector boundaries.  Valid only in scan mode.";
            sp.info->get_config("hashdb_scan_sector_size", &hashdb_scan_sector_size,
                                ss_hashdb_scan_sector_size.str());

            // hashdb_import_sector_size
            std::stringstream ss_hashdb_import_sector_size;
            ss_hashdb_import_sector_size
                << "Selects the import sector size.  Imports along\n"
                << "      sector boundaries.  Valid only in import mode.";
            sp.info->get_config("hashdb_import_sector_size", &hashdb_import_sector_size,
                                ss_hashdb_import_sector_size.str());

            // hashdb_import_repository_name
            std::stringstream ss_hashdb_import_repository_name;
            ss_hashdb_import_repository_name
                << "Sets the repository name to\n"
                << "      attribute the import to.  Valid only in import mode.";
            sp.info->get_config("hashdb_import_repository_name",
                                &hashdb_import_repository_name,
                                ss_hashdb_import_repository_name.str());

            // hashdb_import_max_duplicates
            std::stringstream ss_hashdb_import_max_duplicates;
            ss_hashdb_import_max_duplicates
                << "The maximum number of duplicates to import\n"
                << "      for a given hash value, or 0 for no limit.  Valid only in import mode.";
            sp.info->get_config("hashdb_import_max_duplicates", &hashdb_import_max_duplicates,
                                ss_hashdb_import_max_duplicates.str());


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

            // hashdb_ignore_empty_blocks
            // checks not performed

            // hashdb_block_size
            if (hashdb_block_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_block_size' is invalid.\n"
                         << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_scan_path_or_socket
            // checks not performed

            // hashdb_scan_sector_size
            if (hashdb_scan_sector_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_scan_sector_size' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // for valid operation, scan sectors must align on hash block boundaries
            if (mode == MODE_SCAN && hashdb_block_size % hashdb_scan_sector_size != 0) {
                std::cerr << "Error: invalid hashdb block size=" << hashdb_block_size
                          << " or hashdb scan sector size=" << hashdb_scan_sector_size << ".\n"
                          << "Sectors must align on hash block boundaries.\n"
                          << "Specifically, hashdb_block_size \% hashdb_scan_sector_size must be zero.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_import_sector_size
            if (hashdb_import_sector_size == 0) {
                std::cerr << "Error.  Value for parameter 'hashdb_import_sector_size' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // for valid operation, import sectors must align on hash block boundaries
            if (mode == MODE_IMPORT && hashdb_block_size % hashdb_import_sector_size != 0) {
                std::cerr << "Error: invalid hashdb block size=" << hashdb_block_size
                          << " or hashdb import sector size=" << hashdb_import_sector_size << ".\n"
                          << "Sectors must align on hash block boundaries.\n"
                          << "Specifically, hashdb_block_size \% hashdb_import_sector_size must be zero.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // hashdb_import_repository_name
            // checks not performed
            // hashdb_import_max_duplicates
            // checks not performed

            // perform setup based on mode
            switch(mode) {
                case MODE_IMPORT: {
                    // set the path to the hashdb
                    hashdb_dir = sp.fs.get_outdir() + "/" + "hashdb.hdb";

                    // create the new hashdb manager for importing
                    // currently, hashdb_dir is required to not exist
                    hashdb = new hashdb_t(hashdb_dir,
                                          hashdb_block_size,
                                          hashdb_import_max_duplicates);

                    // show relavent settable options
                    std::string temp1((hashdb_ignore_empty_blocks) ? "YES" : "NO");
                    std::cout << "hashdb: hashdb_mode=" << hashdb_mode << "\n"
                              << "hashdb: hashdb_block_size=" << hashdb_block_size << "\n"
                              << "hashdb: hashdb_ignore_empty_blocks=" << temp1 << "\n"
                              << "hashdb: hashdb_import_sector_size= " << hashdb_import_sector_size << "\n"
                              << "hashdb: hashdb_import_repository_name= " << hashdb_import_repository_name << "\n"
                              << "hashdb: hashdb_import_max_duplicates=" << hashdb_import_max_duplicates << "\n"
                              << "hashdb: Creating hashdb directory " << hashdb_dir << "\n";
                    return;
                }

                case MODE_SCAN: {
                    // show relavent settable options
                    std::string temp2((hashdb_ignore_empty_blocks) ? "YES" : "NO");
                    std::cout << "hashdb: hashdb_mode=" << hashdb_mode << "\n"
                              << "hashdb: hashdb_block_size=" << hashdb_block_size << "\n"
                              << "hashdb: hashdb_ignore_empty_blocks=" << temp2 << "\n"
                              << "hashdb: hashdb_scan_path_or_socket=" << hashdb_scan_path_or_socket << "\n"
                              << "hashdb: hashdb_scan_sector_size=" << hashdb_scan_sector_size << "\n";

                    // open the hashdb manager for scanning
                    hashdb = new hashdb_t(hashdb_scan_path_or_socket);
                    return;
                }

                case MODE_NONE: {
                    // show relavent settable options
                    std::cout << "hashdb: hashdb_mode=" << hashdb_mode << "\n"
                              << "WARNING: the hashdb scanner is enabled but it will not perform any action\n"
                              << "because no mode has been selected.  Please either select a hashdb mode or\n"
                              << "leave the hashdb scanner disabled to avoid this warning.\n";

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

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // there should be at least one block to process
    if (sbuf.pagesize < hashdb_block_size) {
      return;
    }

    // get count of blocks to process
    size_t count = sbuf.bufsize / hashdb_import_sector_size;
    while ((count * hashdb_import_sector_size) +
           (hashdb_block_size - hashdb_import_sector_size) > sbuf.pagesize) {
      --count;
    }

    // allocate space on heap for import_input
    std::vector<hashdb_t::import_element_t>* import_input =
       new std::vector<hashdb_t::import_element_t>;

    // import all the cryptograph hash values from all the blocks in sbuf
    for (size_t i=0; i < count; ++i) {

        // calculate the offset associated with this index
        size_t offset = i * hashdb_import_sector_size;

        // ignore empty blocks
        if (hashdb_ignore_empty_blocks && is_empty_block(sbuf.buf + offset)) {
            continue;
        }

        // calculate the hash for this sector-aligned hash block
        hash_t hash = hash_generator::hash_buf(
                                 sbuf.buf + offset,
                                 hashdb_block_size);

        // compose the filename based on the forensic path
        std::stringstream ss;
        size_t p=sbuf.pos0.path.find('/');
        if (p==std::string::npos) {
            // no directory in forensic path so explicitly include the filename
            ss << sp.fs.get_input_fname();
            if (sbuf.pos0.isRecursive()) {
                // forensic path is recursive so add "/" + forensic path
                ss << "/" << sbuf.pos0.path;
            }
        } else {
            // directory in forensic path so print forensic path as is
            ss << sbuf.pos0.path;
        }

        // calculate the offset from the start of the media image
        uint64_t image_offset = sbuf.pos0.offset + offset;

        // create and add the import element to the import input
        import_input->push_back(hashdb_t::import_element_t(
                                 hash,
                                 hashdb_import_repository_name,
                                 ss.str(),
                                 image_offset));
    }

    // perform the import
    int status = hashdb->import(*import_input);

    if (status != 0) {
        std::cerr << "scan_hashdb import failure\n";
    }

    // clean up
    delete import_input;
}

// perform scan
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb) {

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // there should be at least one block to process
    if (sbuf.pagesize < hashdb_block_size) {
      return;
    }

    // get count of blocks to process
    size_t count = sbuf.bufsize / hashdb_scan_sector_size;
    while ((count * hashdb_scan_sector_size) +
           (hashdb_block_size - hashdb_scan_sector_size) > sbuf.pagesize) {
      --count;
    }

    // allocate space on heap for scan_input
    std::vector<hash_t>* scan_input = new std::vector<hash_t>;

    // allocate space on heap for the offset lookup table
    std::vector<uint32_t>* offset_lookup_table = new std::vector<uint32_t>;

    // get the cryptograph hash values of all the blocks along
    // sector boundaries from sbuf
    for (size_t i=0; i<count; ++i) {

        // calculate the offset associated with this index
        size_t offset = i * hashdb_scan_sector_size;

        // ignore empty blocks
        if (hashdb_ignore_empty_blocks && is_empty_block(sbuf.buf + offset)) {
            continue;
        }

        // add the offset to the offset lookup table
        offset_lookup_table->push_back(offset);

        // calculate and add the hash to the scan input
        scan_input->push_back(hash_generator::hash_buf(
                    sbuf.buf + offset, hashdb_block_size));
    }

    // allocate space on heap for scan_output
    hashdb_t::scan_output_t* scan_output = new hashdb_t::scan_output_t;

    // perform the scan
    int status = hashdb->scan(*scan_input, *scan_output);

    if (status != 0) {
        std::cerr << "Error: scan_hashdb scan failure.  Aborting.\n";
        exit(1);
    }

    // get the feature recorder
    feature_recorder* identified_blocks_recorder = sp.fs.get_name("identified_blocks");

    // record each feature returned in the response
    for (hashdb_t::scan_output_t::const_iterator it=scan_output->begin(); it!= scan_output->end(); ++it) {

        // prepare forensic path (pos0, feature, context)
        // as (pos0, hash_string, count_string)

        // pos0
        pos0_t pos0 = sbuf.pos0 + offset_lookup_table->at(it->first);

        // hash_string
        std::string hash_string = scan_input->at(it->first).hexdigest();

        // count
        std::stringstream ss;
        ss << it->second;
        std::string count_string = ss.str();

        // record the feature
        identified_blocks_recorder->write(pos0, hash_string, count_string);
    }

    // clean up
    delete scan_input;
    delete offset_lookup_table;
    delete scan_output;
}

// detect if block is empty
inline bool is_empty_block(const uint8_t *buf) {
    for (size_t i=1; i<hashdb_block_size; i++) {
        if (buf[i] != buf[0]) {
            return false;
        }
    }
    return true;
}

#endif

