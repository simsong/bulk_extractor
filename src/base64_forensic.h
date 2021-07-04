/*
 * Base64 conversion.
 *
 * From RFC1521 and draft-ietf-dnssec-secext-03.txt.
 *
 * Implementation (C) 1996-1999 by Internet Software Consortium.
 */



#ifndef BASE64_H
#define BASE64_H

#include <cctype>

/* Convert from printable base64 to binary.
 * Returns number of bytes converted
 */
int b64_pton_forensic(const char *str,int srclen,unsigned char *target,size_t targsize);

#endif
