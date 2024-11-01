#include <iostream>
#include <fstream>

#include <swf.h>

using namespace SWFRecomp;

using std::ifstream;
using std::ios_base;

int main()
{
	ifstream swf_file("sup.swf", ios_base::in | ios_base::binary);
	if (!swf_file.good())
	{
		fprintf(stderr, "swf file not found\n");
		return -1;
	}
	
	swf_file.seekg(0, ios_base::end);
	size_t swf_size = swf_file.tellg();
	swf_file.seekg(0, ios_base::beg);
	
	char* swf_buffer = new char[swf_size];
	swf_file.read(swf_buffer, swf_size);
	swf_file.close();
	
	SWFHeader header = SWFHeader(swf_buffer);
	
	delete[] swf_buffer;
}