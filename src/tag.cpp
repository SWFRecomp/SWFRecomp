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
	
	void SWFTag::parseHeader(char*& tag_buffer)
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
	}
	
	void SWFTag::setFieldCount(u32 new_field_count)
	{
		if (fields != nullptr)
		{
			delete[] fields;
		}
		
		field_count = new_field_count;
		
		fields = new SWFField[field_count];
	}
	
	void SWFTag::configureNextField(FieldType type, u32 bit_length, bool is_nbits)
	{
		if (next_field == field_count)
		{
			EXC("Configuring field past the field count.");
		}
		
		fields[next_field].type = type;
		fields[next_field].bit_length = bit_length;
		fields[next_field].is_nbits = is_nbits;
		
		next_field += 1;
	}
	
	void SWFTag::parseFields(char*& tag_buffer, u32 nbits)
	{
		u32 cur_byte_bits_left = 8;
		bool prev_was_bitfield = false;
		
		for (u32 field_i = 0; field_i < field_count; ++field_i)
		{
			fields[field_i].parse(tag_buffer, nbits, cur_byte_bits_left, prev_was_bitfield);
		}
		
		if (cur_byte_bits_left != 8)
		{
			tag_buffer += 1;
		}
	}
	
	void SWFTag::parseFieldsContinue(char*& tag_buffer, u32& cur_byte_bits_left, u32 nbits)
	{
		bool prev_was_bitfield = false;
		
		for (u32 field_i = 0; field_i < field_count; ++field_i)
		{
			fields[field_i].parse(tag_buffer, nbits, cur_byte_bits_left, prev_was_bitfield);
		}
	}
	
	void SWFTag::clearFields()
	{
		next_field = 0;
	}
};