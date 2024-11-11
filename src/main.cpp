#include <iostream>

#include <recompilation.hpp>

using SWFRecomp::recompile;

int main()
{
	recompile("sup_manual.swf", "RecompiledTags", "RecompiledScripts");
}