#pragma once

#include <common.h>

#include <field.hpp>

namespace SWFRecomp
{
	enum SWFTagType
	{
		SWF_TAG_END_TAG = 0,
		SWF_TAG_FILE_ATTRIBUTES = 69,
	};
	
	class SWFTag
	{
	public:
		u8 code;
		u32 nbits;
		u32 length;
		
		u32 field_count;
		u32 next_field;
		SWFField* fields;
		
		SWFTag();
		~SWFTag();
		
		char* parseHeader(char* tag_buffer);
		void setFieldCount(u32 new_field_count);
		void configureNextField(FieldType type, u32 length = 0, bool is_nbits = false);
		char* parseFields(char* tag_buffer);
	};
};