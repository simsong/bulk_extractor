#ifndef TEST_BE_H
#define TEST_BE_H

/* We assume that the tests are being run out of bulk_extractor/src/.
 * This returns the directory of the test subdirectory.
 */

const char *notify();
std::filesystem::path test_dir();
extern bool debug;

// return file the test directory mapped to an sbuf
sbuf_t *map_file(std::filesystem::path p);


// look for specific output in a file, and throw an exception if it cannot be found
void grep(const std::string str, std::filesystem::path fname );
void grep(const Feature &exp, std::filesystem::path fname );

std::filesystem::path test_scanners(const std::vector<scanner_t *> & scanners, sbuf_t *sbuf);
std::filesystem::path test_scanner(scanner_t scanner, sbuf_t *sbuf);
bool requireFeature(const std::vector<std::string> &lines, const std::string feature);

extern const std::string JSON1;
extern const std::string JSON2;

int argv_count(const char **argv);
int run_be(std::ostream &cout, std::ostream &cerr, const char **argv);
int run_be(std::ostream &ss, const char **argv);


#endif
