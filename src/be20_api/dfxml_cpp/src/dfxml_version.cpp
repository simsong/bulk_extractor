/*
 * This file exists for the purpose of creating a .o file so that we do not get errors from libtool when it tries to make a linkable shared library.
 *
 * (C) 2021 Simson L. Garfinkel
 * https://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 */




#include "dfxml_config.h"
#include "dfxml_reader.h"

const char *dfxml::dfxml_version()
{
    return PACKAGE_VERSION;
}
