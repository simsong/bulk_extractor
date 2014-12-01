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
static size_t hashdb_scan_max_features = 0;                     // scan only
static size_t hashdb_import_sector_size = 4096;                 // import only
static std::string hashdb_import_repository_name="default_repository"; // import only
static uint32_t hashdb_import_max_duplicates=0;                 // import only

// runtime modes
// scanner mode
enum mode_type_t {MODE_NONE, MODE_SCAN, MODE_IMPORT};
static mode_type_t mode = MODE_NONE;

// global state

// hashdb directory, import only
static std::string hashdb_dir;

// hash type
typedef md5_t hash_t;
typedef md5_generator hash_generator;

// hashdb manager
typedef hashdb_t__<hash_t> hashdb_t;
static hashdb_t* hashdb;

static void do_import(const class scanner_params &sp,
                      const recursion_control_block &rcb);
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb);


// safely hash sbuf range without overflow failure
inline hash_t hash_one_block(const sbuf_t &sbuf)
{
    if (sbuf.bufsize >= hashdb_block_size) {
        // hash from the beginning
        return hash_generator::hash_buf(sbuf.buf, hashdb_block_size);
    }
    // hash the available part and zero-fill
    hash_generator g;
    g.update(sbuf.buf, sbuf.bufsize);

    // hash in extra zeros to fill out the block
    size_t extra = hashdb_block_size - sbuf.bufsize;
    std::vector<uint8_t> zeros(extra);
    g.update(&zeros[0], extra);
    return g.final();
}

// rules for determining if a block should be ignored
static bool ramp_trait(const sbuf_t &sbuf)
{
    if (sbuf.pagesize < 8) {
        // not enough to process
        return false;
    }

    uint32_t count = 0;
    for(size_t i=0;i<sbuf.pagesize-8;i+=4){
        // note that little endian is detected and big endian is not detected
        if (sbuf.get32u(i)+1 == sbuf.get32u(i+4)) {
            count += 1;
        }
    }
    return count > sbuf.pagesize/8;
}

static bool hist_trait(const sbuf_t &sbuf)
{
    if (sbuf.pagesize < hashdb_block_size) {
        // do not perform any histogram analysis on short blocks
        return false;
    }

    std::map<uint32_t,uint32_t> hist;
    for(size_t i=0;i<sbuf.pagesize-4;i+=4){
        hist[sbuf.get32uBE(i)] += 1;
    }
    if (hist.size() < 3) return true;
    for (std::map<uint32_t,uint32_t>::const_iterator it = hist.begin();it != hist.end(); it++){
        if ((it->second) > hashdb_block_size/16){
            return true;
        }
    }
    return false;
}

static bool whitespace_trait(const sbuf_t &sbuf)
{
    size_t count = 0;
    for(size_t i=0;i<sbuf.pagesize;i++){
        if (isspace(sbuf[i])) count+=1;
    }
    return count >= (sbuf.pagesize * 3)/4;
}

// detect if block is all the same
inline bool empty_sbuf(const sbuf_t &sbuf)
{
    for (size_t i=1; i<sbuf.bufsize; i++) {
        if (sbuf[i] != sbuf[0]) {
            return false;
        }
    }
    return true;                        // all the same
}

extern "C"
void scan_hashdb(const class scanner_params &sp,
                 const recursion_control_block &rcb) {

    switch(sp.phase) {
        // startup
        case scanner_params::PHASE_STARTUP: {

            // set properties for this scanner
            std::string desc = "Search cryptographic hash IDs against hashes in a hashdb block hash database";
            desc += std::string(" (hashdb version") + std::string(hashdb_version()) + std::string(")");

            sp.info->name        = "hashdb";
            sp.info->author      = "Bruce Allen";
            sp.info->description = desc;
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

            // hashdb_scan_max_features
            std::stringstream ss_hashdb_scan_max_features;
            ss_hashdb_scan_max_features
                << "The maximum number of features lines to record\n"
                << "      or 0 for no limit.  Valid only in scan mode.";
            sp.info->get_config("hashdb_scan_max_features", &hashdb_scan_max_features,
                                ss_hashdb_scan_max_features.str());

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
                          << hashdb_mode << "' must be [none|import|scan].\n"
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

            // hashdb_scan_max_features
            // checks not performed

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

                    // open hashdb for importing
                    // currently, hashdb_dir is required to not exist
                    hashdb = new hashdb_t();
                    std::pair<bool, std::string> open_import_pair = hashdb->open_import(
                                          hashdb_dir,
                                          hashdb_block_size,
                                          hashdb_import_max_duplicates);
                    if (open_import_pair.first == false) {
                        std::cerr << "Error opening hashdb for importing.\n"
                                  << open_import_pair.second << "\nAborting.\n";
                        exit(1);
                    }

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
                              << "hashdb: hashdb_scan_sector_size=" << hashdb_scan_sector_size << "\n"
                              << "hashdb: hashdb_scan_max_features=" << hashdb_scan_max_features << "\n";

                    // open hashdb for scanning
                    hashdb = new hashdb_t();
                    std::pair<bool, std::string> open_scan_pair = hashdb->open_scan(
                                          hashdb_scan_path_or_socket);
                    if (open_scan_pair.first == false) {
                        std::cerr << "Error opening hashdb for scanning.\n"
                                  << open_scan_pair.second << "\nAborting.\n";
                        exit(1);
                    }
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
            delete hashdb;
            return;
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

    // allocate space on heap for import_input
    std::vector<hashdb_t::import_element_t>* import_input =
                                 new std::vector<hashdb_t::import_element_t>;

    // the filename from sbuf without the sbuf map file delimiter
    std::string path_without_map_file_delimiter =
              (sbuf.pos0.path.size() > 4) ?
              std::string(sbuf.pos0.path, 0, sbuf.pos0.path.size() - 4) : "";
 
    // get the filename to use for source attribution
    std::stringstream ss;
    size_t p=sbuf.pos0.path.find('/');
    if (p==std::string::npos) {
        // no directory in forensic path so explicitly include the filename
        ss << sp.fs.get_input_fname();
        if (sbuf.pos0.isRecursive()) {
            // forensic path is recursive so add "/" + forensic path
            ss << "/" << path_without_map_file_delimiter;
        }
    } else {
        // directory in forensic path so print forensic path as is
        ss << path_without_map_file_delimiter;
    }
    std::string filename = ss.str();

    // import the cryptograph hash values from all the blocks in sbuf
    for (size_t offset=0; offset<sbuf.pagesize; offset+=hashdb_import_sector_size) {

        // Create a child sbuf of what we would hash
        const sbuf_t sbuf_to_hash(sbuf,offset,hashdb_block_size);

        // ignore empty blocks
        if (hashdb_ignore_empty_blocks && empty_sbuf(sbuf_to_hash)){
            continue;
        }

        // calculate the hash for this import-sector-aligned hash block
        hash_t hash = hash_one_block(sbuf_to_hash);

        // calculate the offset from the start of the media image
        uint64_t image_offset = sbuf_to_hash.pos0.offset;

        // create and add the import element to the import input
        import_input->push_back(hashdb_t::import_element_t(
                                 hash,
                                 hashdb_import_repository_name,
                                 filename,
                                 image_offset));
    }

    // perform the import
    int status = hashdb->import(*import_input);

    if (status != 0) {
        std::cerr << "scan_hashdb import failure\n";
    }

    // clean up import input buffer
    delete import_input;

    // calculate the sbuf hash for the source metadata
    hash_t sbuf_hash = hash_generator::hash_buf(sbuf.buf, sbuf.pagesize);

    // store the source metadata
    hashdb->import_metadata(hashdb_import_repository_name,
                            filename,
                            sbuf.pagesize,
                            sbuf_hash);
}

// perform scan
static void do_scan(const class scanner_params &sp,
                    const recursion_control_block &rcb) {

    // get the feature recorder
    feature_recorder* identified_blocks_recorder = sp.fs.get_name("identified_blocks");

    // optimization: don't even scan if feature line count is at requested max
    if (hashdb_scan_max_features > 0 && identified_blocks_recorder->count() >=
                                                   hashdb_scan_max_features) {
        return;
    }

    // get the sbuf
    const sbuf_t& sbuf = sp.sbuf;

    // allocate space on heap for scan_input
    std::vector<hash_t>* scan_input = new std::vector<hash_t>;

    // allocate space on heap for the offset lookup table
    std::vector<uint32_t>* offset_lookup_table = new std::vector<uint32_t>;

    // process cryptographic hash values for blocks along sector boundaries
    for (size_t offset=0; offset<sbuf.pagesize; offset+=hashdb_scan_sector_size) {

        // Create a child sbuf of what we would hash
        const sbuf_t sbuf_to_hash(sbuf,offset,hashdb_block_size);

        // ignore empty blocks
        if (hashdb_ignore_empty_blocks && empty_sbuf(sbuf_to_hash)){
            continue;
        }

        // add the offset to the offset lookup table
        offset_lookup_table->push_back(offset);

        // calculate the hash for this scan-sector-aligned hash block
        hash_t hash = hash_one_block(sbuf_to_hash);

        // calculate and add the hash to the scan input
        scan_input->push_back(hash);
    }

    // allocate space on heap for scan_output
    hashdb_t::scan_output_t* scan_output = new hashdb_t::scan_output_t;

    // perform the scan
    int status = hashdb->scan(*scan_input, *scan_output);

    if (status != 0) {
        std::cerr << "Error: scan_hashdb scan failure.  Aborting.\n";
        exit(1);
    }

    // record each feature returned in the response
    for (hashdb_t::scan_output_t::const_iterator it=scan_output->begin(); it!= scan_output->end(); ++it) {

        // stop recording if feature line count is at requested max
        if (hashdb_scan_max_features > 0 && identified_blocks_recorder->count() >=
                                                   hashdb_scan_max_features) {
            break;
        }

        // prepare forensic path (pos0, feature, context)
        // as (pos0, hash_string, count_string)

        // pos0
        size_t offset = offset_lookup_table->at(it->first);
        pos0_t pos0 = sbuf.pos0 + offset;

        // hash_string
        std::string hash_string = scan_input->at(it->first).hexdigest();

        // set flags based on specific tests on the block
        // Construct an sbuf from the block and subject it to the other tests
        const sbuf_t s(sbuf, offset,hashdb_block_size);
        std::stringstream ss_flags;
        if (ramp_trait(s)) ss_flags << "R";
        if (hist_trait(s)) ss_flags << "H";
        if (whitespace_trait(s)) ss_flags << "W";

        // build context field containing count and flags
        std::stringstream ss;
        ss << "{\"count\":" << it->second;
        if (ss_flags.str().size() > 0) {
            // show flags too
            ss << ",\"flags\":\"" << ss_flags.str() << "\"";
        }
        ss << "}";

        // record the feature
        identified_blocks_recorder->write(pos0, hash_string, ss.str());
    }

    // clean up
    delete scan_input;
    delete offset_lookup_table;
    delete scan_output;
}


#endif

