#ifndef SCAN_CCNS2_H
#define SCAN_CCNS2_H
/* scan_ccns2.cpp --- here because it's used in both scan_accts.flex and scan_ccns2.cpp
 */
bool  valid_ccn(const char *buf,int buflen);
bool  valid_phone(const sbuf_t &sbuf,size_t pos,size_t len);
bool  valid_bitcoin_address(const char *buf,size_t buflen);
void  build_unbase58();
bool  unbase58(const char *s,uint8_t *out,size_t len);
extern int scan_ccns2_debug;
#endif
