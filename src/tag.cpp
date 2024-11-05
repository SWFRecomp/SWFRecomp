#include <tag.hpp>

namespace SWFRecomp
{
	SWFTag::SWFTag()
	{
		
	}
	
	char* SWFTag::parseHeader(char* tag_buffer)
	{
		u16 tag_code_and_length = *((u16*) tag_buffer);
		tag_buffer += 2;
		
		tag_code = (tag_code_and_length >> 6) & 0x3FF;
		length = tag_code_and_length & 0b111111;
		
		if (length == 0x3F)
		{
			length = *((u32*) tag_buffer);
			tag_buffer += 4;
		}
		
		return tag_buffer;
	}
	
	char* SWFTag::parseFields(char* tag_buffer)
	{
		tag_buffer += length;
		
		return tag_buffer;
	}
};