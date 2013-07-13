#ifndef PYXPRESS_H
#define PYXPRESS_H

/* pyxpress.c */

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef __BEGIN_DECLS
#if defined(__cplusplus)
#define __BEGIN_DECLS   extern "C" {
#define __END_DECLS     }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif



__BEGIN_DECLS
unsigned long Xpress_Decompress(const unsigned char *InputBuffer,
                                unsigned long InputSize,
                                unsigned char *OutputBuffer,
                                unsigned long OutputSize);
__END_DECLS

#endif
