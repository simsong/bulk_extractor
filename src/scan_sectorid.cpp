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
 * Generates MD5 hash values from chunk_size data taken along sector_hash
 * boundaries and scans for matches against a hash database.
 *
 * Note that the hash database may be accessed locally through the
 * file system or remotely through a socket.
 */

#include "bulk_extractor.h"

#ifdef HAVE_SECTORID

#include "sector_hash_query.hpp"
#include <dfxml/src/hash_t.h>

#include <iostream>
#include <unistd.h>	// for getpid
#include <sys/types.h>	// for getpid

// static values that can be set from config
static size_t chunk_size = 4096;
static size_t sector_size = 512;
static sector_hash::lookup_type_t lookup_type = sector_hash::QUERY_NOT_SELECTED;
static std::string lookup_type_string = lookup_type_to_string(lookup_type);
std::string client_hashdb_path = "a valid hashdb directory path is required";
std::string client_socket_endpoint = "tcp://localhost:14500";

static bool scanner_is_usable = true;

// the sector hash query service
static sector_hash::sector_hash_query_t* query = 0;

extern "C"
void scan_sectorid(const class scanner_params &sp,
                   const recursion_control_block &rcb) {

    switch(sp.phase) {
        // startup
        case scanner_params::PHASE_STARTUP: {

            // set properties for this scanner
            sp.info->name        = "sectorid";
            sp.info->author      = "Bruce Allen";
            sp.info->description = "Search sector IDs, specifically, search MD5 hashes against hashes in a MD5 hash database";

            // import lookup_type
            std::stringstream help_lookup_type;
            help_lookup_type << "      <lookup_type> used to perform the lookup, where <lookup_type>\n"
                             << "      is one of use_path | use_socket (default "
                                                          << lookup_type_to_string(lookup_type) << ")\n"
                             << "      use_path   - Lookups are performed from a hashdb in the filesystem\n"
                             << "                   at the specified <path>.\n"
                             << "      use_socket - Lookups are performed from a server service at the\n"
                             << "                   specified <socket>.\n"
                             ;
            sp.info->get_config("lookup_type", &lookup_type_string, help_lookup_type.str());

            // import path
            std::stringstream help_path;
            help_path        << "      Specifies the <path> to the hash database to be used for performing\n"
                             << "      the lookup service.  This option is only used when the lookup type\n"
                             << "      is set to \"use_path\".\n"
                             ;
            sp.info->get_config("path", &client_hashdb_path, help_path.str());

            // import socket
            std::stringstream help_socket;
            help_socket      << "      Specifies the client <socket> endpoint to use to connect with the\n"
                             << "      sector_hash server (default '" << client_socket_endpoint << "').  Valid socket\n"
                             << "      transports supported by the zmq messaging kernel are tcp, ipc, and\n"
                             << "      inproc.  Currently, only tcp is tested.  This opition is only valid\n"
                             << "      when the lookup type is set to \"lookup_socket\".\n"
                             ;
            sp.info->get_config("socket", &client_socket_endpoint, help_socket.str());

            // import chunk_size
            sp.info->get_config("chunk_size", &chunk_size, "Chunk size, in bytes, used to generate hashes");

            // import sector_size
            std::stringstream help_sector_size;
            help_sector_size << "Sector size, in bytes\n";
            help_sector_size << "      Hashes are generated on each sector_size boundary.\n";
            sp.info->get_config("sector_size", &sector_size, help_sector_size.str());

            // configure the feature file if a usable query type is selected
            sector_hash::lookup_type_t temp_lookup_type;
            bool temp_is_valid __attribute__ ((unused)) = string_to_lookup_type(lookup_type_string, temp_lookup_type);
            if (temp_lookup_type != sector_hash::QUERY_NOT_SELECTED) {
                sp.info->feature_names.insert("identified_blocks");
            }

            return;
        }

        // init
        case scanner_params::PHASE_INIT: {

            // validate lookup_type
            bool is_valid = string_to_lookup_type(lookup_type_string, lookup_type);
            if (!is_valid) {
                std::cerr << "Error.  Value '" << lookup_type_string
                          << "' for parameter 'lookup_type' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // validate chunk_size
            if (chunk_size == 0) {
                std::cerr << "Error.  Value for parameter 'chunk_size' is invalid.\n"
                         << "Cannot continue.\n";
                exit(1);
            }

            // validate sector_size
            if (sector_size == 0) {
                std::cerr << "Error.  Value for parameter 'sector_size' is invalid.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // also, for valid operation, sectors must align on chunk boundaries
            if (chunk_size % sector_size != 0) {
                std::cerr << "Error: invalid chunk size=" << chunk_size
                          << " or sector size=" << sector_size << ".\n"
                          << "Sectors must align on chunk boundaries.\n"
                          << "Specifically, sectorid_chunk_size \% sectorid_sector_size must be zero.\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // perform setup based on selected query type
            switch(lookup_type) {
                case sector_hash::QUERY_USE_PATH:
                    query = new sector_hash::sector_hash_query_t(lookup_type, client_hashdb_path);
                    break;
                case sector_hash::QUERY_USE_SOCKET:
                    query = new sector_hash::sector_hash_query_t(lookup_type, client_socket_endpoint);
                    break;
                default:
                    scanner_is_usable = false;
            }
            return;
        }

        // scan
        case scanner_params::PHASE_SCAN: {
            if (!scanner_is_usable) {
                return;
            }

            // get the feature recorder
            feature_recorder* md5_recorder = sp.fs.get_name("identified_blocks");

            // get the sbuf
            const sbuf_t& sbuf = sp.sbuf;

            // allocate space for request and response
            sector_hash::hashes_request_md5_t request;
            sector_hash::hashes_response_md5_t response;

            // populate request with chunk hashes calculated from sbuf
            // use i as query id so that later it can be used as the feature
            // offset
            for (size_t i=0; i + chunk_size <= sbuf.pagesize; i += chunk_size) {
                // calculate the hash
                md5_t md5 = md5_generator::hash_buf(sbuf.buf + i, chunk_size);

                // convert md5 to uint8_t[]
                // note: could optimize and typecast instead.
                uint8_t digest[16];
                memcpy(digest, md5.digest, 16);

                // add the hash to the lookup hash request
                request.hash_requests.push_back(
                                  sector_hash::hash_request_md5_t(i, digest));
            }

            // perform the lookup
            bool success = query->lookup_hashes_md5(request, response);

            if (!success) {
                // the lookup failed
                std::cerr << "Error in scan_sectorid hash lookup\n";
                return;
            }

            // always make sure the server is using the same chunk size
            if (response.chunk_size != chunk_size) {
                std::cerr << "Error: The scanner is hashing using a chunk size of " << chunk_size << "\n"
                          << "but the hashdb contains hashes for data of chunk size " << response.chunk_size << ".\n"
                          << "Cannot continue.\n";
                exit(1);
            }

            // record each feature in the response
            for (std::vector<sector_hash::hash_response_md5_t>::const_iterator it = response.hash_responses.begin(); it != response.hash_responses.end(); ++it) {

                // get the variables together for the feature
                pos0_t pos0 = sbuf.pos0 + it->id;

                // convert uint8_t[] to md5
                md5_t md5;
                memcpy(md5.digest,it->digest, 16);

                std::string feature = md5.hexdigest();
                stringstream ss;
                ss << it->duplicates_count;
                std::string context = ss.str();

                // record the feature
                md5_recorder->write(pos0, feature, context);
            }

            return;
        }

        // shutdown
        case scanner_params::PHASE_SHUTDOWN: {
            if (!scanner_is_usable) {
                return;
            }

            // deallocate hashdb query service resources
            delete query;
            return;
        }

        // there are no other states
        default: {
            // no action for other states
            return;
        }
    }
}

#endif

