#include <cstdio>

#include <tag.hpp>

namespace SWFRecomp
{
	SWFTag::SWFTag() : field_count(0), next_field(0), fields(nullptr)
	{
		
	}
	
	SWFTag::~SWFTag()
	{
		if (fields != nullptr)
		{
			delete[] fields;
		}
	}
	
	char* SWFTag::parseHeader(char* tag_buffer)
	{
		u16 code_and_length = *((u16*) tag_buffer);
		tag_buffer += 2;
		
		code = (TagType) ((code_and_length >> 6) & 0x3FF);
		length = code_and_length & 0b111111;
		
		if (length == 0x3F)
		{
			length = *((u32*) tag_buffer);
			tag_buffer += 4;
		}
		
		return tag_buffer;
	}
	
	void SWFTag::setFieldCount(u32 new_field_count)
	{
		if (field_count >= new_field_count)
		{
			return;
		}
		
		if (fields != nullptr)
		{
			delete[] fields;
		}
		
		field_count = new_field_count;
		
		fields = new SWFField[field_count];
	}
	
	void SWFTag::configureNextField(FieldType type, u32 bit_length, bool is_nbits)
	{
		fields[next_field].type = type;
		next_field += 1;
	}
	
	char* SWFTag::parseFields(char* tag_buffer)
	{
		for (u32 field_i = 0; field_i < field_count; ++field_i)
		{
			tag_buffer = fields[field_i].parse(tag_buffer);
		}
		
		return tag_buffer;
	}
	
	void SWFTag::clearFields()
	{
		next_field = 0;
	}
};