//#include "bulk_extractor_api.h"

#include <iostream>

extern "C"
void hello(const char *param)
{
    std::cout << "Received from caller: " << param << "\n";
}
