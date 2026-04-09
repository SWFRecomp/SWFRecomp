#include <iostream>

#include <common.h>
#include <config.hpp>
#include <recompilation.hpp>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Not enough arguments.\nusage: %s <config-file>\n", argv[0]);
		return -1;
	}
	
	SWFRecomp::Context context;
	context.config.parseFile(argv[1]);
	
	SWFRecomp::recompile(context);
	
	fflush(stdout);
	
	return 0;
}