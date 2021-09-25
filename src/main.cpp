/*
 * main.cpp
 *
 * The main() for bulk_extractor.
 * This has all of the code and global variables that aren't needed when BE is running as a library.
 */

#include "config.h"
#include "bulk_extractor.h"

#include <ostream>

int main(int argc,char * const *argv)
{
    return bulk_extractor_main(std::cout, std::cerr, argc, argv);
}
