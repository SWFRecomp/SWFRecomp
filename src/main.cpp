#include <iostream>

#include <recompilation.hpp>

using SWFRecomp::recompile;

int main()
{
	recompile("sup.swf", "RecompiledTags", "RecompiledScripts");
}