#pragma once

#include <common.h>

namespace SWFRecomp
{
	class SWFTag
	{
	public:
		u8 tag_code;
		u32 length;
		
		SWFTag();
		
		char* parseHeader(char* tag_buffer);
		char* parseFields(char* tag_buffer);
	};
};