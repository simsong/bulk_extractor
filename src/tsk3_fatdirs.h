#ifndef TSK3_FATDIRS_H
#define TSK3_FATDIRS_H

/* We have a private version of these #include files in case the system one is not present */
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include tsk3/libtsk.h"
#include "tsk3/fs/tsk_fatfs.h"
#include "tsk3/fs/tsk_ntfs.h"
#pragma GCC diagnostic warning "-Wshadow"
#pragma GCC diagnostic warning "-Weffc++"
#pragma GCC diagnostic warning "-Wredundant-decls"

/**
 * code from tsk3
 */

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
#endif
