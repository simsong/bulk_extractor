#ifndef TEST_BE_H
#define TEST_BE_H

/* We assume that the tests are being run out of bulk_extractor/src/.
 * This returns the directory of the test subdirectory.
 */

std::filesystem::path test_dir();

// return file the test directory mapped to an sbuf
sbuf_t *map_file(std::filesystem::path p);

#endif
