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
	
	SWFRecomp::Config config;
	config.parseFile(argv[1]);
	
	SWFRecomp::Context context;
	context.swf_path = config.swf_path;
	context.output_tags_folder = "RecompiledTags";
	context.output_scripts_folder = "RecompiledScripts";
	
	SWFRecomp::recompile(context);
	
	fflush(stdout);
	
	return 0;
}