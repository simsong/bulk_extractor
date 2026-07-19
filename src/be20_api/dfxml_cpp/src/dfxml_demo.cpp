/*
 * DFXML demo program.
 *
 * Simson L. Garfinkel
 *
 * Revision History:
 * 2021 - Cleaned up. Added LGPL copyright notice.
 *
 * Copyright (C) 2021 Simson L. Garfinkel.
 *
 * LICENSE: LGPL Version 3. See COPYING.md for further information.
 *
 */


#include "dfxml_config.h"
#include "dfxml_reader.h"

void process(dfxml::file_object &fi)
{
    std::cout << "fi.filename: " << fi.filename() << "\n";
    std::cout << "  pieces: " << fi.byte_runs.size() << "\n";
}


int main(int argc,char **argv)
{
    dfxml::file_object_reader::read_dfxml(argv[1],process);
    return 0;
}
