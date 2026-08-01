/*
 * main.cpp
 *
 * The main() for bulk_extractor.
 * This has all of the code and global variables that aren't needed when BE is running as a library.
 */

#include "config.h"
#include "bulk_extractor.h"

#include <exception>
#include <ostream>

int main(int argc,char * const *argv)
{
    try {
        return bulk_extractor_main(std::cout, std::cerr, argc, argv);
    } catch (const std::exception &error) {
        std::cerr << "bulk_extractor: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "bulk_extractor: unexpected exception\n";
    }
    return 1;
}
