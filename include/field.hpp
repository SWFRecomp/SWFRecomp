#pragma once

#include <common.h>

namespace SWFRecomp
{
	enum FieldType
	{
		SWF_FIELD_NONE,
		
		SWF_FIELD_SI8,
		SWF_FIELD_SI16,
		SWF_FIELD_SI32,
		
		SWF_FIELD_SI8_ARR,
		SWF_FIELD_SI16_ARR,
		
		SWF_FIELD_UI8,
		SWF_FIELD_UI16,
		SWF_FIELD_UI32,
		
		SWF_FIELD_UI8_ARR,
		SWF_FIELD_UI16_ARR,
		SWF_FIELD_UI24_ARR,
		SWF_FIELD_UI32_ARR,
		SWF_FIELD_UI64_ARR,
		
		SWF_FIELD_FIXED,
		SWF_FIELD_FIXED8,
		
		SWF_FIELD_FLOAT16,
		SWF_FIELD_FLOAT,
		SWF_FIELD_DOUBLE,
		
		SWF_FIELD_ENCODEDU32,
		
		SWF_FIELD_SB,
		SWF_FIELD_UB,
		SWF_FIELD_FB,
		
		SWF_FIELD_STRING
	};
	
	class SWFField
	{
	public:
		u32 bit_length;
		FieldType type;
		s64 value;
		bool is_nbits;
		
		SWFField();
		
		void parse(char*& field_buffer, u32& nbits, u32& cur_byte_bits_left, bool& prev_was_bitfield);
		void parseBitField(char*& field_buffer, u32 nbits, u32& cur_byte_bits_left, bool sb);
	};
};