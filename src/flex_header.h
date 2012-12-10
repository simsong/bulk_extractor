/* flex_header.h:
 *
 * bulk_extractor header which is prepended to the gnuflex output by Makefile.am.
 *
 * We can't have an #include at the beginning of the .flex file because
 * gnuflex puts stuff before the first line.
 */

#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
