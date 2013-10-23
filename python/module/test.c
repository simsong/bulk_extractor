#include<stdlib.h>
#include<stdio.h>

#include "fakelib.h"

int main()
{
    be_handle* handle = bulk_extractor_open();
    printf("%p\n", handle);
    return 0;
}
