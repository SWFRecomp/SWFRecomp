#pragma once

#include <common.h>

#include <field.hpp>

namespace SWFRecomp
{
	enum SWFTagType
	{
		SWF_TAG_END_TAG = 0,
		SWF_TAG_SHOW_FRAME = 1,
		SWF_TAG_SET_BACKGROUND_COLOR = 9,
		SWF_TAG_ENABLE_DEBUGGER_2 = 64,
		SWF_TAG_SCRIPT_LIMITS = 65,
		SWF_TAG_FILE_ATTRIBUTES = 69,
		SWF_TAG_SYMBOL_CLASS = 76,
		SWF_TAG_METADATA = 77,
		SWF_TAG_DO_ABC = 82
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
		void configureNextField(FieldType type, u32 bit_length = 0, bool is_nbits = false);
		char* parseFields(char* tag_buffer);
	};
};