#include "linelist_file.h"
#include <assert.h>

const char *names[] = {"donotkeep@company.com","simson@keep.com","keep@company.net",
		       "donotkeep@mit.edu","justme@justme.com","nobody@nowhere","president@ussr.org",0};

int main(int argc,char **argv)
{
    myregex jj(".*@company.com",REG_ICASE);
    int match = jj.search("simson@company.com");
    printf("match for simson@company.com=%d\n",match);
    assert(match!=0);
    match = jj.search("simson@mit.edu");
    printf("match for simson@mit.edu=%d\n",match);
    assert(match==0);
    linelist_file f;

    if(f.readfile("test_linelist.txt")){
	fprintf(stderr,"Cannot read test_linelist.txt\n");
	exit(1);
    }
    for(int i=0;names[i];i++){
	if(f.contains(names[i])) printf("contains %s\n",names[i]);
    }
    printf("\n");
    for(int i=0;names[i];i++){
	if(!f.contains(names[i])) printf("does not contain %s\n",names[i]);
    }
}
