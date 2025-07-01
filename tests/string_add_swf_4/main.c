#include <recomp.h>

extern frame_func frame_funcs[];

int main()
{
	swfStart(frame_funcs);
}