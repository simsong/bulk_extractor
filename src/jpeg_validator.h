/*
 * jpeg_validator.h:
 * go over a set of bytes and determine if the file follows the JPEG packing conventions.
 * Uses sbuf to hold the data
 */

#ifndef JPEG_VALIDTOR_H
#define JPEG_VALIDTOR_H

#include "be20_api/sbuf.h"

extern bool exif_debug;

#define EDEBUG(x) if (exif_debug) std::cerr << x << "\n";

struct jpeg_validator {
    jpeg_validator(){};
    static const uint32_t MAX_ENTRY_SIZE = 1024*1024*16; // don't look at JPEGs bigger than 16MiB
    static const uint32_t MIN_JPEG_SIZE  = 200;    // don't consider something smaller than this
    typedef enum { UNKNOWN=0,COMPLETE=1,TRUNCATED=2,CORRUPT=-1 } how_t;
    struct results_t {
        results_t():len(),how(),seen_ff_c4(),seen_ff_cc(),seen_ff_db(),
                  seen_ff_e1(),seen_JFIF (),seen_EOI(),seen_SOI(),seen_SOS(),height(),width(){}
        ssize_t len;
        how_t how;	       // how do we validate?
        bool seen_ff_c4;            // Seen a DHT (Definition of Huffman Tables)
        bool seen_ff_cc;            // Seen a DAC (Definition of arithmetic coding)
        bool seen_ff_db;            // Seen a DQT (Definition of quantization)
        bool seen_ff_e1;            // seen an E1 (exif)
        bool seen_JFIF;
        bool seen_EOI;
        bool seen_SOI;
        bool seen_SOS;
        uint16_t height;
        uint16_t width;
    };
    static struct results_t validate_jpeg(const sbuf_t &sbuf) {
        if (exif_debug) std::cerr << "validate_jpeg " << sbuf << "\n";
        results_t res;
        res.how = UNKNOWN;
        size_t i = 0;
        while(i+1 < sbuf.bufsize && res.seen_EOI==false && res.how!=CORRUPT && res.how!=COMPLETE && res.how!=TRUNCATED){
            if (sbuf[i]!=0xff){
                if (exif_debug) std::cerr << "sbuf[" << i << "]=" << (int)(sbuf[i]) << "CORRUPT 1\n";
                res.how = CORRUPT;
                break;
            }
            switch(sbuf[i+1]){                // switch on the marker
            case 0xff: // FF FF is very bad...
                EDEBUG("CORRUPT FF FF");
                res.how = CORRUPT;
                break;
            case 0xd8: // SOI -- start of image
                EDEBUG("SOI");
                res.seen_SOI = true;
                i+=2;
                break;
            case 0x01: // TEM
                EDEBUG("TEM");
                res.how = COMPLETE;
                i+=2;
                break;
            case 0xd0: case 0xd1: case 0xd2: case 0xd3:
            case 0xd4: case 0xd5: case 0xd6: case 0xd7:
                // RST markers are standalone; they don't have lengths
                EDEBUG("RST");
                i += 2;
                break;
            case 0xd9: // EOI- end of image
                EDEBUG("EOI");
                res.seen_EOI = true;
                res.how = COMPLETE;
                i += 2;
                break;
            default: // decode variable-length blocks
                {
                    if (i+8 >= sbuf.bufsize){i+=8;break;} // whoops - not enough
                    uint16_t block_length = sbuf.get16uBE(i+2);
                    if (sbuf[i+1]==0xc4) res.seen_ff_c4 = true;
                    if (sbuf[i+1]==0xcc) res.seen_ff_cc = true;
                    if (sbuf[i+1]==0xdb) res.seen_ff_db = true;
                    if (sbuf[i+1]==0xe1) res.seen_ff_e1 = true;

                    if (sbuf[i+1]==0xe0 && block_length>8){        // see if this is a JFIF
                        EDEBUG("JFIF");
                        if (sbuf[i+4]=='J' && sbuf[i+5]=='F' && sbuf[i+6]=='I' && sbuf[i+7]=='F'){
                            res.seen_JFIF = true;
                        }
                    }

                    if (sbuf[i+1]==0xc0 && block_length>8){        // FFC0 is start of frame
                        res.height = sbuf.get16uBE(i+5);
                        res.width  = sbuf.get16uBE(i+7);
                    }

                    i += 2 + sbuf.get16uBE(i+2);    // add variable length size
                    break;
                }

            case 0xda: // Start of scan
                EDEBUG("SOS");
                /* Certain fields MUST be set before the SOS, or the JPEG is corrupt */
                res.seen_SOS = true;
                if (res.seen_ff_c4==false && res.seen_ff_cc==false && res.seen_ff_db==false){
                    res.how = CORRUPT;         // can't have a SOS before we have seen one of these
                    res.len = 0;               // it's not a jpeg
                    return res;
                }
                if (res.seen_JFIF==false || res.width==0 || res.height==0){
                    res.how = CORRUPT;
                    res.len = 0;
                    return res;
                }

                // http://webtweakers.com/swag/GRAPHICS/0143.PAS.html
                //printf("start of scan. i=%zd header=%d\n",i,sbuf.get16uBE(i+2));
                if (i+2 >= sbuf.bufsize){i+=2;break;}
                i += 2 + sbuf.get16uBE(i+2);   // skip ff da and minor header info

                // Image data follows
                // Scan for EOI or an unescaped invalid FF
                for(;i+1 < sbuf.bufsize;i++){
                    if (sbuf[i]!=0xff){  // Non-FF can be skipped
                        continue;
                    }
                    if (sbuf[i+1]==0x00){ // escaped FF
                        continue;
                    }
                    if (sbuf[i+1]==0xde){ // terminated by an EOI marker
                        if (exif_debug) fprintf(stderr,"i=%zd FF DE EOI found\n",i);
                        res.how = COMPLETE;
                        i+=2;
                        break;
                    }
                    if (sbuf[i+1]==0xd9){ // terminated by an EOI marker
                        if (exif_debug) fprintf(stderr,"i=%zd FF D9 EOI found\n",i);
                        i+=2;
                        res.how = COMPLETE;
                        break;
                    }
                    if (sbuf[i+1]>=0xc0 && sbuf[i+1]<=0xdf){
                        continue;   // This range seems to continue valid control characters
                    }
                    if (exif_debug) fprintf(stderr," ** WTF? sbuf[%zd+1]=%2x\n",i,sbuf[i+1]);
                    res.how = CORRUPT;
                    break;
                }
                // ran off the end in the stream. Fall through below.
                if (exif_debug) std::cerr << "END OF STREAM\n";
                goto done;
            }
        } /* while */
    done:;
        // The JPEG is incomplete.
        // We ran off the end *before* we entered the SOS. We may be in the Exif, but we have
        // nothing displayable.
        res.len = i;            // that's how far we got
        if (res.seen_JFIF==false || res.seen_SOI==false){
            res.how = CORRUPT;
            res.len = 0;        // nothing here, I guess
            return res;
        }

        // If we got an EXIF, at least return that...
        if (res.seen_ff_e1){
            res.how = TRUNCATED;
            return res; // at least we saw an EXIF, so return true
        }

        // If we didn't get a color table, it's corrupt
        if (res.seen_ff_c4==false && res.seen_ff_cc==false && res.seen_ff_db==false){
            res.how = CORRUPT;
            res.len = 0;                    // doesn't appear to be a jpeg
            return res;
        }

        // If we didn't get a SOS or the size is wrong, it's corrupt
        if (res.seen_SOS==false || res.width==0 || res.height==0){
            res.how = CORRUPT;
            res.len = 0;
            return res;
        }
        return res;
    }
};
#endif
