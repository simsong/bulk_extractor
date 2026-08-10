#ifndef TEST_BE_H
#define TEST_BE_H

#include <array>
#include <cstddef>

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
void grep(const Feature &exp, std::filesystem::path fname, size_t delta=0);

std::filesystem::path test_scanners(const std::vector<scanner_t *> & scanners, sbuf_t *sbuf);
std::filesystem::path test_scanner(scanner_t scanner, sbuf_t *sbuf);
constexpr std::array<size_t, 2> SCANNER_TEST_OFFSETS {0, 65};
bool position_shifted_by(const std::string &before, const std::string &after, size_t delta);
void verify_shifted_feature_positions(const std::filesystem::path &baseline,
                                      const std::filesystem::path &shifted,
                                      size_t delta);
bool requireFeature(const std::vector<std::string> &lines, const std::string feature);

extern const std::string JSON1;
extern const std::string JSON2;

int argv_count(const char **argv);
int run_be(std::ostream &cout, std::ostream &cerr, const char **argv);
int run_be(std::ostream &ss, const char **argv);


#endif
