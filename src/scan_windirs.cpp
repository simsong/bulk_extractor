/**
 * Plugin: scan_windirs
 * Purpose: scan for Microsoft directory and MFT structures
 * FAT32 directories always start on sector boundaries. 
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>
#include <sys/time.h>

#include "utf8.h"
#include "dfxml/src/dfxml_writer.h"


/* We have a private version of these #include files in case the system one is not present */
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include "tsk3/libtsk.h"
#include "tsk3/fs/tsk_fatfs.h"
#include "tsk3/fs/tsk_ntfs.h"
#pragma GCC diagnostic warning "-Wshadow"
#pragma GCC diagnostic warning "-Weffc++"
#pragma GCC diagnostic warning "-Wredundant-decls"

#define CLUSTERS_IN_1MiB 2*1024
#define CLUSTERS_IN_1GiB 2*1024*1024

/* fat32 tuning parameters for weirdness. Each of these define something weird. If too much is weird, it's probably not a FAT32 directory entry.. */
static uint32_t opt_weird_file_size    = 1024*1024*150; // max file size
static uint32_t opt_weird_file_size2   = 1024*1024*512; // max file size
static uint32_t opt_weird_cluster_count    = 32*CLUSTERS_IN_1GiB; // assume smaller than 32GB with 512 byte clusters
static uint32_t opt_weird_cluster_count2   = 128*CLUSTERS_IN_1GiB; // assume smaller than 512GB with 512 byte clusters
static uint32_t opt_max_bits_in_attrib = 3;
static uint32_t opt_max_weird_count    = 2;
static uint32_t opt_last_year = 2020;
static int  debug=0;

/**
 * code from tsk3
 */

//using namespace std;


inline uint16_t fat16int(const uint8_t buf[2]){
    return buf[0] | (buf[1]<<8);
}

inline uint32_t fat32int(const uint8_t buf[4]){
    return buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
}

inline uint32_t fat32int(const uint8_t high[2],const uint8_t low[2]){
    return low[0] | (low[1]<<8) | (high[0]<<16) | (high[1]<<24);
}


inline int fatYear(int x){  return (x & FATFS_YEAR_MASK) >> FATFS_YEAR_SHIFT;}
inline int fatMonth(int x){ return (x & FATFS_MON_MASK) >> FATFS_MON_SHIFT;}
inline int fatDay(int x){   return (x & FATFS_DAY_MASK) >> FATFS_DAY_SHIFT;}
inline int fatHour(int x){  return (x & FATFS_HOUR_MASK) >> FATFS_HOUR_SHIFT;}
inline int fatMin(int x){   return (x & FATFS_MIN_MASK) >> FATFS_MIN_SHIFT;}
inline int fatSec(int x){   return (x & FATFS_SEC_MASK) >> FATFS_SEC_SHIFT;}

inline std::string fatDateToISODate(const uint16_t d,const uint16_t t)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
	     fatYear(d)+1980,fatMonth(d),fatDay(d),
	     fatHour(t),fatMin(t),fatSec(t)); // local time
    return std::string(buf);
}



/* validate an 8.3 name (not a long file name) */
bool valid_fat_dentry_name(const uint8_t name[8],const uint8_t ext[3])
{
    if( name[0]=='.' && name[1]==' ' && name[2]==' ' && name[3]==' '
	&& name[4]==' ' && name[5]==' ' && name[6]==' ' && name[7]==' '
	&& ext[0]==' ' && ext[1]==' ' && ext[2]==' ') return true;

    if( name[0]=='.' && name[1]=='.' && name[2]==' ' && name[3]==' '
	&& name[4]==' ' && name[5]==' ' && name[6]==' ' && name[7]==' '
	&& ext[0]==' ' && ext[1]==' ' && ext[2]==' ') return true;

    for(int i=0;i<8;i++){
	if(!FATFS_IS_83_NAME(name[i])) return false; // invalid name
    }

    for(int i=0;i<3;i++){
	if(!FATFS_IS_83_EXT(ext[i])) return false; // invalid exension
    }

    /* make sure all characters are valid*/
    for(int i=0;i<8;i++){
        const uint8_t ch = name[i];
        if(ch==0 || ch==' ') break;     // end of name
        if(!isupper(ch) && !isdigit(ch) && ch!=' ' && ch!='!' && ch!='#' &&
           ch != '$' && ch!='%' && ch !='&' && ch !='\'' && ch!='(' && ch!=')' &&
           ch != '-' && ch!='@' && ch != '^' && ch!='_' && ch!='`' && ch!='{' && ch !='}' && ch!='~'){
            return false;
        }
    }

    for(int i=0;i<3;i++){
        const uint8_t ch = ext[i];
        if(ch==0 || ch==' ') break;     // end of name
        if(!isupper(ch) && !isdigit(ch) && ch!=' ' && ch!='!' && ch!='#' &&
           ch != '$' && ch!='%' && ch !='&' && ch !='\'' && ch!='(' && ch!=')' &&
           ch != '-' && ch!='@' && ch != '^' && ch!='_' && ch!='`' && ch!='{' && ch !='}' && ch!='~'){
            return false;
        }
    }

    return true;
}


enum fat_validation_t {
    INVALID=0,
    VALID_DENTRY=1,
    VALID_LFN=2,
    VALID_LAST_DENTRY=10,
    ALL_NULL=20} ;

static uint16_t fat_year(short f)
{
    return ((f & FATFS_YEAR_MASK) >> FATFS_YEAR_SHIFT) + 1980;
}

// http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
inline uint32_t count_bits(unsigned x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003F;
}


/**
 * Return 0 if the directory is invalid
 * 1 if the directory is valid dentry
 * 2 if the directory is valid LFN
 * 10 if the directory is valid and it's the last in the sector.
 * 20 if all null, so there are no more valid
 *
 * http://en.wikipedia.org/wiki/File_Allocation_Table
 */
fat_validation_t valid_fat_directory_entry(const sbuf_t &sbuf)
{
    if(sbuf.bufsize != sizeof(fatfs_dentry)) return INVALID; // not big enough
    /* If the entire directory entry is the same character, it's not valid */
    if(sbuf.is_constant(sbuf[0])) return ALL_NULL; // clearly not valid

    const fatfs_dentry &dentry = *(sbuf.get_struct_ptr<fatfs_dentry>(0));
    if((dentry.attrib & ~FATFS_ATTR_ALL) != 0) return INVALID; // invalid attribute bit set
    if(dentry.attrib == FATFS_ATTR_LFN){
	/* This may be a VFAT long file name */
	const fatfs_dentry_lfn &lfn = *(const fatfs_dentry_lfn *)sbuf.buf;
	if((lfn.seq & ~0x40) > 10) return INVALID;	// invalid sequence number
	if(lfn.reserved1 != 0) return INVALID;          // invalid reserved1 (LDIR_Type)
	if(fat16int(lfn.reserved2)!=0) return INVALID;  // LDIR_FstClusLO "Must be ZERO"
	return VALID_LFN;			        // looks okay
    } else {
	if(dentry.name[0]==0) return VALID_LAST_DENTRY; // "Entry is available and no subsequent entry is in use. "

	/* Look for combinations of times, dates and attributes that have been invalid */
	if((dentry.attrib & FATFS_ATTR_LFN)==FATFS_ATTR_LFN &&
	   (dentry.attrib != FATFS_ATTR_LFN)){
	    return INVALID;			// LFN set but DIR or ARCHIVE is also set
	}
	if((dentry.attrib & FATFS_ATTR_DIRECTORY) && (dentry.attrib & FATFS_ATTR_ARCHIVE)){
	    return INVALID;			// can't have both DIRECTORY and ARCHIVE set
	}

        if(dentry.attrib & 0x40) return INVALID; // "Device, never found on disk" (wikipedia)

	if(!valid_fat_dentry_name(dentry.name,dentry.ext)) return INVALID; // invalid name
	if(dentry.ctimeten>199) return INVALID;	// create time fine resolution, 0..199
	uint16_t ctime = fat16int(dentry.ctime);
	uint16_t cdate = fat16int(dentry.cdate);
	uint16_t adate = fat16int(dentry.adate);
	uint16_t wtime = fat16int(dentry.wtime);
	uint16_t wdate = fat16int(dentry.wdate);

	if(ctime && !FATFS_ISTIME(ctime)) return INVALID; // ctime is null for directories
	if(cdate && !FATFS_ISDATE(cdate)) return INVALID; // cdate is null for directories
	if(adate && !FATFS_ISDATE(adate)) return INVALID; // adate is null for directories
	if(adate==0 && ctime==0 && cdate==0){
	    if(dentry.attrib & FATFS_ATTR_VOLUME) return VALID_DENTRY; // volume name
	    return INVALID;					    // not a volume name
	}
	if(!FATFS_ISTIME(wtime)) return INVALID; // invalid wtime
	if(!FATFS_ISDATE(wdate)) return INVALID; // invalid wdate
	if(ctime && ctime==cdate) return INVALID; // highly unlikely
	if(wtime && wtime==wdate) return INVALID; // highly unlikely
	if(adate && adate==ctime) return INVALID; // highly unlikely
	if(adate && adate==wtime) return INVALID; // highly unlikely

        /* Look for things that are weird in a FAT32 entry.
         * This is configurable and largely based on inspection of false-positives.
         * The parameters should be learned through machine learning, of course...
         */
        uint16_t weird_count = 0;
        if(fat_year(cdate) > opt_last_year) weird_count++;
        if(fat_year(adate) > opt_last_year) weird_count++;
        if(fat32int(dentry.size) > opt_weird_file_size) weird_count++;
        if(fat32int(dentry.size) > opt_weird_file_size2) weird_count++;
        if(count_bits(dentry.attrib) > opt_max_bits_in_attrib) weird_count++;
        if(fat32int(dentry.highclust,dentry.startclust) > opt_weird_cluster_count) weird_count++;
        if(fat32int(dentry.highclust,dentry.startclust) > opt_weird_cluster_count2) weird_count++;
        if(dentry.ctimeten != 0 && dentry.ctimeten != 100) weird_count++;
        if(adate==0 && cdate==0) weird_count++;
        if(adate==0 && wdate==0) weird_count++;

        if(weird_count > opt_max_weird_count) return INVALID;
                                                                           
    }
    return VALID_DENTRY;
}


void scan_fatdirs(const sbuf_t &sbuf,feature_recorder *wrecorder)
{
    /* 
     * Directory structures are 32 bytes long and will always be sector-aligned.
     * So try every 512 byte sector, within that try every 32 byte record.
     */
    
    for(size_t base = 0;base<sbuf.pagesize;base+=512){
	sbuf_t sector(sbuf,base,512);
	if(sector.bufsize < 512){
	    return;			// no space left
	}

	int last_valid_entry_number = -1;
	int ret1_count = 0;
	int valid_year_count = 0;
	const int max_entries = 512/32;
	int slots[max_entries];
	memset(slots,0,sizeof(slots));
	for(ssize_t entry_number = 0; entry_number < max_entries; entry_number++){
	    sbuf_t n(sector,entry_number*32,32);
	    
	    int ret = valid_fat_directory_entry(n);
	    if(ret==ALL_NULL) break;		// no more valid
	    slots[entry_number] = ret;
	    if(ret==VALID_DENTRY){
		/* Attempt to validate the years */
		const fatfs_dentry &dentry = *n.get_struct_ptr<fatfs_dentry>(0);
		uint16_t ayear = fatYear(fat16int(dentry.adate));
		uint16_t cyear = fatYear(fat16int(dentry.cdate));
		uint16_t wyear = fatYear(fat16int(dentry.wdate));

		if( (ayear==0 || ((int)1980+ayear < (int)opt_last_year))
		    && (cyear==0 || ((int)1980+cyear < (int)opt_last_year))
		    && ((int)1980+wyear < (int)opt_last_year)){
			valid_year_count++;
		}
		ret1_count++;
	    }
	    if(ret==INVALID){			// invalid; they are all bad
		//last_valid_entry_number = -1; // found an invalid directory entry
		break;
	    }
	    if(ret==VALID_DENTRY || ret==VALID_LFN){	// valid; go to the next
		last_valid_entry_number = entry_number;
		continue;
	    }
	    if(ret==VALID_LAST_DENTRY){		// valid; no more remain
		last_valid_entry_number = entry_number;
		break;		
	    }
	}
	/* Now print the valid entry numbers */
	if(ret1_count==1 && valid_year_count==0) continue; // year is bogus
	if(last_valid_entry_number==1 && valid_year_count==0) continue; // year is bogus
	if(last_valid_entry_number>=0 && ret1_count>0){
	    for(ssize_t entry_number = 0;entry_number <= last_valid_entry_number && entry_number<max_entries;
		entry_number++){
		sbuf_t n(sector,entry_number*32,32);
		dfxml_writer::strstrmap_t fatmap;
		
		if(valid_fat_directory_entry(n)==1){
		    const fatfs_dentry &dentry = *n.get_struct_ptr<fatfs_dentry>(0);
		    std::stringstream ss;
		    for(int j=0;j<8;j++){ if(dentry.name[j]!=' ') ss << dentry.name[j]; }
		    ss << ".";
		    for(int j=0;j<3;j++){ if(dentry.ext[j]!=' ') ss << dentry.ext[j]; }
		    std::string filename = ss.str();
		    fatmap["filename"] = filename;
		    fatmap["ctimeten"] = utos((uint32_t)dentry.ctimeten);
		    fatmap["ctime"]    = fatDateToISODate(fat16int(dentry.cdate),fat16int(dentry.ctime));
		    fatmap["atime"]    = fatDateToISODate(fat16int(dentry.adate),0);
		    fatmap["mtime"]    = fatDateToISODate(fat16int(dentry.wdate),fat16int(dentry.wtime));
		    fatmap["startcluster"] = utos(fat32int(dentry.highclust,dentry.startclust));
		    fatmap["filesize"] = utos(fat32int(dentry.size));
		    fatmap["attrib"]   = utos((uint32_t)dentry.attrib);
		    wrecorder->write(n.pos0,filename,dfxml_writer::xmlmap(fatmap,"fileobject","src='fat'"));
		}
	    }
	}
    }
}

/**
 * Examine an sbuf and see if it contains an NTFS MFT entry. If it does, then process the entry 
 */
void scan_ntfsdirs(const sbuf_t &sbuf,feature_recorder *wrecorder)
{
    for(size_t base = 0;base<sbuf.pagesize;base+=512){
	sbuf_t n(sbuf,base,1024);
	std::string filename;
	if(n.bufsize!=1024){
	    continue;	// no space
	}
	try{
	    if(n.get32u(0)==NTFS_MFT_MAGIC){ // NFT magic number matches
		if(debug & DEBUG_INFO) n.hex_dump(std::cerr);

		uint16_t nlink = n.get16u(16); // get link count
		if(nlink<10){ // sanity check - most files have less than 10 links

		    dfxml_writer::strstrmap_t mftmap;
		    mftmap["nlink"] = utos(nlink);
		    mftmap["lsn"]   = utos(n.get64u(8)); // $LogFile Sequence Number
		    mftmap["seq"]   = utos(n.get16u(18));
		    size_t attr_off = n.get16u(20); // don't make 16bit!

		    // Now look at every attribute for the ones that we care about

		    int found_attrs = 0;
		    while(attr_off+sizeof(ntfs_attr) < n.bufsize){

			uint32_t attr_type = n.get32u(attr_off+0);
			uint32_t attr_len  = n.get32u(attr_off+4);

			if(debug & DEBUG_INFO){
			    std::cerr << "---------------------\n";
			    n.hex_dump(std::cerr,attr_off,128);
			    std::cerr << " attr_off=" << attr_off << " attr_type=" << attr_type
				      << " attr_len=" << attr_len;
			}
		    
			if(attr_len==0){
			    if(debug & DEBUG_INFO) std::cerr << "\n";
			
			    break;	// something is wrong; skip this entry
			}
		    
			// get the values for all entries
			int  res         = n.get8u(attr_off+8);
			size_t nlen      = n.get8u(attr_off+9);
			size_t name_off  = n.get16u(attr_off+10);
			uint32_t mft_flags = n.get16u(attr_off+12);
			uint32_t id        = n.get16u(attr_off+14);

			if(debug & DEBUG_INFO){
			    std::cerr << " res=" << (int)res << " nlen=" << (int)nlen << " name_off="
				      << name_off << " mft_flags="<< mft_flags << " id=" << id << "\n";
			}

			if(res!=NTFS_MFT_RES){ // we can only handle resident attributes
			    attr_off += attr_len;
			    continue;
			}

			if(attr_type==NTFS_ATYPE_ATTRLIST){
			    found_attrs++;
			    if(debug & DEBUG_INFO) std::cerr << "NTFS_ATTRLIST ignored\n";
			}
		    
			if(attr_type==NTFS_ATYPE_FNAME ){ 
			    found_attrs++;
			    if(debug & DEBUG_INFO) std::cerr << "NTFS_ATYPE_FNAME\n";

			    // Decode a resident FNAME fields
			    // Previously all of the get16u's were put into uint16_t, but that
			    // turned out to cause overflow problems, so don't do that.

			    size_t soff         = n.get16u(attr_off+20);

			    mftmap["par_ref"] = utos(n[attr_off+soff+0]
						     | (n[attr_off+soff+1]<<8)
						     | (n[attr_off+soff+2]<<16)
						     | (n[attr_off+soff+3]<<24)
						     | ((uint64_t)n[attr_off+soff+4]<<32)
						     | ((uint64_t)n[attr_off+soff+5]<<40));
			    mftmap["par_seq"] = utos(n.get16u(attr_off+soff+6));
			    mftmap["crtime_fn"] = microsoftDateToISODate(n.get64u(attr_off+soff+8));
			    mftmap["mtime_fn"]  = microsoftDateToISODate(n.get64u(attr_off+soff+16));
			    mftmap["ctime_fn"]  = microsoftDateToISODate(n.get64u(attr_off+soff+24));
			    mftmap["atime_fn"]  = microsoftDateToISODate(n.get64u(attr_off+soff+32));

			    // these can be sanity checked

			    static const uint64_t terabyte = uint64_t(1000) * uint64_t(1000) * uint64_t(1000) * uint64_t(1000);
			    uint64_t filesize_alloc = n.get64u(attr_off+soff+40);
			    if(filesize_alloc > (1000L * terabyte)) break;

			    mftmap["filesize_alloc"] = utos(filesize_alloc);

			    uint64_t filesize = n.get64u(attr_off+soff+48);
			    if(filesize > (1000L * terabyte)) break;
			    mftmap["filesize"]       = utos(filesize);

			    mftmap["attr_flags"]     = utos(n.get64u(attr_off+soff+56));
			    size_t  fname_nlen   = n.get8u(attr_off+soff+64);
			    size_t  fname_nspace = n.get8u(attr_off+soff+65);
			    size_t  fname_npos   = attr_off+soff+66;

			    if(debug & DEBUG_INFO) std::cerr << " soff=" << soff << " fname_nlen=" << fname_nlen
							     << " fname_nspace=" << fname_nspace
							     << " fname_npos=" << fname_npos
							     << " (" << fname_npos-attr_off << "-attr_off) "
							     << "\n";

			    std::wstring utf16str;
			    for(size_t i=0;i<fname_nlen;i++){ // this is pretty gross; is there a better way?
				utf16str.push_back(n.get16u(fname_npos+i*2));
			    }
			    filename = safe_utf16to8(utf16str);
			    mftmap["filename"] = filename;
			}

			if(attr_type==NTFS_ATYPE_SI){
			    found_attrs++;
			    if(debug & DEBUG_INFO) std::cerr << "NTFS_ATYPE_SI\n";

			    size_t soff         = n.get16u(attr_off+20);

			    mftmap["crtime_si"] = microsoftDateToISODate(n.get64u(attr_off+soff+0));
			    mftmap["mtime_si"]  = microsoftDateToISODate(n.get64u(attr_off+soff+8));
			    mftmap["ctime_si"]  = microsoftDateToISODate(n.get64u(attr_off+soff+16));
			    mftmap["atime_si"]  = microsoftDateToISODate(n.get64u(attr_off+soff+24));
			}

                        if(attr_type==NTFS_ATYPE_OBJID){
                            found_attrs++;
                            if(debug & DEBUG_INFO) std::cerr << "NTFS_ATYPE_OBJID\n";

                            size_t slen         = n.get32u(attr_off+16);
                            size_t soff         = n.get16u(attr_off+20);
                            char guid_objid[37], guid_bvolid[37], guid_bobjid[37], guid_domid[37];

			    if(debug & DEBUG_INFO) std::cerr << " soff=" << soff << " slen=" << slen
							     << "\n";    

                            if(slen>=16){
                                snprintf(guid_objid, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", \
                                    n[attr_off+soff+3], n[attr_off+soff+2], n[attr_off+soff+1], n[attr_off+soff+0],      \
                                    n[attr_off+soff+5], n[attr_off+soff+4], n[attr_off+soff+7], n[attr_off+soff+6],      \
                                    n[attr_off+soff+8], n[attr_off+soff+9], n[attr_off+soff+10], n[attr_off+soff+11],    \
                                    n[attr_off+soff+12], n[attr_off+soff+13], n[attr_off+soff+14], n[attr_off+soff+15]);
                                mftmap["guid_objectid"] = guid_objid;
                            }
                            if(slen>=32){
                                soff+=16;
                                snprintf(guid_bvolid, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", \
                                    n[attr_off+soff+3], n[attr_off+soff+2], n[attr_off+soff+1], n[attr_off+soff+0],      \
                                    n[attr_off+soff+5], n[attr_off+soff+4], n[attr_off+soff+7], n[attr_off+soff+6],      \
                                    n[attr_off+soff+8], n[attr_off+soff+9], n[attr_off+soff+10], n[attr_off+soff+11],    \
                                    n[attr_off+soff+12], n[attr_off+soff+13], n[attr_off+soff+14], n[attr_off+soff+15]);
                                mftmap["guid_birthvolumeid"] = guid_bvolid;
                            }
                            if(slen>=48){
                                soff+=16;
                                snprintf(guid_bobjid, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", \
                                    n[attr_off+soff+3], n[attr_off+soff+2], n[attr_off+soff+1], n[attr_off+soff+0],      \
                                    n[attr_off+soff+5], n[attr_off+soff+4], n[attr_off+soff+7], n[attr_off+soff+6],      \
                                    n[attr_off+soff+8], n[attr_off+soff+9], n[attr_off+soff+10], n[attr_off+soff+11],    \
                                    n[attr_off+soff+12], n[attr_off+soff+13], n[attr_off+soff+14], n[attr_off+soff+15]);
                                mftmap["guid_birthobjectid"] = guid_bobjid;
                            }
                            if(slen>=64){
                                soff+=16;
                                snprintf(guid_domid, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", \
                                    n[attr_off+soff+3], n[attr_off+soff+2], n[attr_off+soff+1], n[attr_off+soff+0],      \
                                    n[attr_off+soff+5], n[attr_off+soff+4], n[attr_off+soff+7], n[attr_off+soff+6],      \
                                    n[attr_off+soff+8], n[attr_off+soff+9], n[attr_off+soff+10], n[attr_off+soff+11],    \
                                    n[attr_off+soff+12], n[attr_off+soff+13], n[attr_off+soff+14], n[attr_off+soff+15]);
                                mftmap["guid_domainid"] = guid_domid;
                            }
                        }
		    
			attr_off += attr_len;
		    }		
		    if(mftmap.size()>3){
			if(filename.size()==0) filename="$NOFILENAME"; // avoids problems
			wrecorder->write(n.pos0,filename,dfxml_writer::xmlmap(mftmap,"fileobject","src='mft'"));
		    }
		    if(debug & DEBUG_INFO) std::cerr << "=======================\n";
		}
	    }
	    //const ntfs_mft &mft = *(const ntfs_mft *)sbuf.buf;
	}
	catch ( sbuf_t::range_exception_t &e ){
	    /**
	     * If we got a range exception, then the region we were reading
	     * can't be a valid MFT entry...
	     */
	    continue;			
	}
    }
}


extern "C"
void scan_windirs(const class scanner_params &sp,const recursion_control_block &rcb)
{
    std::string myString;
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){

        /* Figure out the current time */
        time_t t = time(0);
        struct tm now;
        memset(&now,0,sizeof(now));     // assures all of now is cleared; required for static analysis tools
        gmtime_r(&t,&now);                     
        opt_last_year = now.tm_year + 1900 + 5; // allow up to 5 years in the future

        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "windirs";
        sp.info->author         = "Simson Garfinkel and Maxim Suhanov";
        sp.info->description    = "Scans Microsoft directory structures";
	sp.info->flags =  scanner_info::SCANNER_DEPTH_0; // only run at top level by default
        sp.info->scanner_version= "1.0";
	sp.info->feature_names.insert("windirs");

        sp.info->get_config("opt_weird_file_size",&opt_weird_file_size,"Threshold for FAT32 scanner");
        sp.info->get_config("opt_weird_file_size2",&opt_weird_file_size2,"Threshold for FAT32 scanner");
        sp.info->get_config("opt_weird_cluster_count",&opt_weird_cluster_count,"Threshold for FAT32 scanner");
        sp.info->get_config("opt_weird_cluster_count2",&opt_weird_cluster_count2,"Threshold for FAT32 scanner");
        sp.info->get_config("opt_max_bits_in_attrib",&opt_max_bits_in_attrib,
                            "Ignore FAT32 entries with more attributes set than this");
        sp.info->get_config("opt_max_weird_count",&opt_max_weird_count,"Number of 'weird' counts to ignore a FAT32 entry");
        sp.info->get_config("opt_last_year",&opt_last_year,"Ignore FAT32 entries with a later year than this");

        debug = sp.info->config->debug;

        

	return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;		// no shutdown
    if(sp.phase==scanner_params::PHASE_SCAN){
	feature_recorder *wrecorder = sp.fs.get_name("windirs");
	scan_fatdirs(sp.sbuf,wrecorder);
	scan_ntfsdirs(sp.sbuf,wrecorder);
    }
}
