#include "scan_exif.h"


int debug=1;
int main(int argc,char **argv)
{
    exif_debug = debug;
    (void)jpeg_carve_mode;
    (void)min_jpeg_size;
    argc--;argv++;
    while(*argv){
        sbuf_t *sbuf = sbuf_t::map_file(*argv, pos0_t(*argv) );
        if(sbuf==0){
            perror(*argv);
        } else {
            jpeg_validator::results_t res = v.validate_jpeg(*sbuf);
            printf("%s: filesize: %zd  s=%zd  how=%d\n",*argv,sbuf->bufsize,res.len,res.how);
            delete sbuf;
        }
        argc--;argv++;
        printf("\n");
    }
    return(0);
}
