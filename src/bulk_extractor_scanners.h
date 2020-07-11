/* compiled-in scanners. 
 *
 * scanners are brought in by defining the SCANNER macro and then including this file.
 */

/* flex-based scanners */
SCANNER(email)  
SCANNER(accts)  
SCANNER(kml)
SCANNER(gps)

/* Regular scanners */
SCANNER(wordlist) 
SCANNER(net) 
SCANNER(base16)
SCANNER(base64)
SCANNER(vcard)
SCANNER(lift)
SCANNER(extx)
SCANNER(find)

#ifdef HAVE_EXIV2
SCANNER(exiv2)
#endif

SCANNER(aes)
SCANNER(bulk)
SCANNER(sceadan)
SCANNER(elf)
SCANNER(exif)
SCANNER(gzip)
SCANNER(hiberfile)
SCANNER(httplogs)
SCANNER(json)
#ifdef HAVE_LIBLIGHTGREP
SCANNER(accts_lg)
SCANNER(base16_lg)
SCANNER(email_lg)
SCANNER(gps_lg)
SCANNER(lightgrep)
#endif
SCANNER(facebook)
SCANNER(ntfsusn)
SCANNER(pdf)
SCANNER(msxml)
SCANNER(winlnk)
SCANNER(winpe)
SCANNER(winprefetch)
SCANNER(zip)
SCANNER(rar)
SCANNER(windirs)
SCANNER(xor)
SCANNER(outlook)
SCANNER(sqlite)

// scanners provided by 4n6ist:
SCANNER(utmp)
SCANNER(ntfsmft)
SCANNER(ntfslogfile)
SCANNER(ntfsindx)
SCANNER(evtx)


