#include "stop_list.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc,char **argv)
{
    stop_lists.add_stoplist(argv[1],false);
    const string *check = stop_lists.check(string(argv[2]),string(argv[2]));
    if(check==0) printf("not found");
    else printf("found: %s\n",check->c_str());
    return(0);
}

