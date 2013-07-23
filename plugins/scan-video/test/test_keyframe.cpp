#include "extract_keyframes.h"
#include "histogram_process.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv){


	extract_keyframes *ek = new extract_keyframes();
	std::string file_name(argv[1]);
	ek->extract(file_name);
	delete ek;
	return 0;
}

