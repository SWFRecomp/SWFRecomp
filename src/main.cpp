#include <iostream>

#include <recompilation.hpp>

using SWFRecomp::recompile;

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Not enough arguments.\nusage: %s <name-of-swf>\n", argv[0]);
		return -1;
	}
	
	recompile(argv[1], "RecompiledTags", "RecompiledScripts");
	
	return 0;
}