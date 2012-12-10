#ifndef SCAN_CCNS2_H
#define SCAN_CCNS2_H
/* scan_ccns2.cpp --- here because it's used in both scan_accts.flex and scan_ccns2.cpp
 */
int  validate_ccn(const char *buf,int buflen);
int  validate_phone(const sbuf_t &sbuf,size_t pos,size_t len);
#endif
