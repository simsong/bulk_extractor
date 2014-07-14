#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>
#include <ctype.h>
#include <stdlib.h>

#include "utf8.h"

// This code is not ready for prime time
// Don't use it
#ifdef ENABLE_SCAN_EXTX			

#ifdef HAVE_TSK3_LIBTSK_H


#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wredundant-decls"

#include "tsk3/libtsk.h"
#include "tsk3/fs/tsk_ext2fs.h"

/**
 * Plugin: scan_extx
 * Purpose: scanner for linux directory and ext structures
 *  typedef struct {
 uint8_t inode[4];       // u32
 uint8_t rec_len[2];     // u16
 uint8_t name_len[2];    // u16
 char name[EXT2FS_MAXNAMLEN];
 } ext2fs_dentry1;

 new structure starting at 2.2
 typedef struct {
 uint8_t inode[4];       // u32
 uint8_t rec_len[2];     // u16
 uint8_t name_len;
 uint8_t type;
 char name[EXT2FS_MAXNAMLEN];
 } ext2fs_dentry2;

*/

/**
 * code from tsk3
 */

using namespace std;

/**
 * Functions: validate_versionX
 * Following functios validates if it is ext2 or 3 (i.e. version 1 or version 2)
 *
 */

bool validate_version1(const ext2fs_dentry1 *dentry1)
{
    bool v1 = true;

    size_t namelen = static_cast< size_t >(*dentry1->name_len);
    char *name = (char *) dentry1->name; // we could say that we have a name

    // Lets check to see if it an actual name
    for(size_t i = 0; (i < namelen) && (v1 == true) && (i < 256); i++){
	if(isprint(name[i]))
	    {
		v1 = true;
	    }
	else{
	    v1 = false;
	}
    }

    return v1;
}

bool validate_version2(const ext2fs_dentry2 *dentry2)
{
    bool v2 = true;

    size_t namelen = static_cast < int > (dentry2->name_len);
    char *name = (char *) dentry2->name; // we could say that we have a name

    // Lets check to see if it an actual name
    for(size_t i = 0; (i < namelen) && (v2 == true) && (i < 256); i++){
	if(isprint(name[i])){
	    v2 = true;
	}
	else{
	    v2 = false;
	}
    }
    return v2;
}

/**
 * validates the dentry:
 * Returns: 1 if dentry type 1, 2 if dentry type 2 otherwise 0 dentry's not found
 *
 */

int validate_x(const ext2fs_dentry1 *dentry1, const ext2fs_dentry2 *dentry2)
{
    int version = 0;
    bool v1 = false;
    bool v2 = false;

    // First check if it is a dentry 1 or 2 by looking at dentry's 2 type field
    // if the type field is valid then it is a dentry 2 and process

    size_t type = static_cast <size_t> (dentry2->type);
	
    if((type == 1) && (!v2)) {
	v2 = validate_version2(dentry2);
	if(v2 == true)
	    version = 2;
    } else {
	v1 = validate_version1(dentry1);
	if(v1 == true)
	    version = 1;
    }

    return version;
}

/**************************************************************************************************
 * Examine an sbuf and see if it contains an ext2 entry. If it does, then process the entry 
 *
 */

void scan_ext1byte(const sbuf_t &sbuf, feature_recorder *extxrecorder)
{
    const ext2fs_dentry1 *dentry1 = sbuf.get_struct_ptr<ext2fs_dentry1>(0);
    const ext2fs_dentry2 *dentry2 = sbuf.get_struct_ptr<ext2fs_dentry2>(0);

    size_t endpoint = sbuf.bufsize - 96;
    // for loop is used to check individual bytes - until we are boundary aligned
    // once boundary aligned then we increment by the record length in the while loop
    for(size_t index = 0; index < endpoint; index++){
	// Case 0: dentry not found not boundary aligned
	// Case 1: ext2fs_dentry type 1 found
	// Case 2: ext2fs_dentry type 2 found

	switch(validate_x(dentry1, dentry2)){
	case 0: // not boundary aligned 
	    {
		//	cout << "dentry not found / not boundary aligned" << endl;
		break;
	    }

	case 1: // dentry type 1 
	    {
		// Simple check after initial detection we will see if we received a space as our initial
		// detection in the name character array - 
		int name = int(dentry1->name[0]);	
		if(name > 0){
		    extxrecorder->write(dentry1->name);
		}	

		size_t reclen = static_cast <size_t> (*dentry1->rec_len);
		index += reclen; 
		break;
	    }
			
	case 2: // dentry type 2
	    {
		extxrecorder->write(dentry2->name);
		size_t reclen = static_cast<size_t> (*dentry2->rec_len);
		index += reclen; 
		break;
	    }
	}
	dentry1 = (ext2fs_dentry1 *) (sbuf.buf + index);
	dentry2 = (ext2fs_dentry2 *) (sbuf.buf + index);
    }
}

#endif

extern "C"
void scan_extx(const class scanner_params &sp,const recursion_control_block &rcb)
{
    string myString;
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "extx";
        sp.info->name		= "extx";
        sp.info->author         = "T.Melaragno & K.Fairbanks";
        sp.info->description    = "Scans extx directory structures";
        sp.info->scanner_version= "1.0";
        sp.info->feature_names.insert("extx");
	sp.info->flags		= scanner_info::SCANNER_DISABLED;
	return;
    }
    if(sp.phase==scanner_params::shutdown) return;		// no shutdown
    if(sp.phase==scanner_params::scan){
#ifdef HAVE_TSK3_LIBTSK_H

  	feature_recorder *extxrecorder = sp.fs.get_name("extx");
	scan_ext1byte(sp.sbuf, extxrecorder);
#endif
    }
}
#endif
