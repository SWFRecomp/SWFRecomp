#include <iostream>

#include <swf.hpp>

using namespace SWFRecomp;

int main()
{
	SWF swf = SWF("sup.swf");
	
	while (swf.parseTag());
}