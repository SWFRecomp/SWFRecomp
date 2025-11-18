#pragma once

#include <common.h>
#include <field.hpp>

namespace SWFRecomp
{
	enum TagType
	{
		SWF_TAG_END_TAG = 0,
		SWF_TAG_SHOW_FRAME = 1,
		SWF_TAG_DEFINE_SHAPE = 2,
		SWF_TAG_DEFINE_BITS = 6,
		SWF_TAG_JPEG_TABLES = 8,
		SWF_TAG_SET_BACKGROUND_COLOR = 9,
		SWF_TAG_DEFINE_FONT = 10,
		SWF_TAG_DEFINE_TEXT = 11,
		SWF_TAG_DO_ACTION = 12,
		SWF_TAG_DEFINE_FONT_INFO = 13,
		SWF_TAG_DEFINE_SHAPE_2 = 22,
		SWF_TAG_PLACE_OBJECT_2 = 26,
		SWF_TAG_ENABLE_DEBUGGER = 58,
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
		TagType code;
		u32 nbits;
		u32 length;
		
		u32 field_count;
		u32 next_field;
		SWFField* fields;
		
		SWFTag();
		~SWFTag();
		
		void parseHeader(char*& tag_buffer);
		void setFieldCount(u32 new_field_count);
		void configureNextField(FieldType type, u32 bit_length = 0, bool is_nbits = false);
		void parseFields(char*& tag_buffer, u32 nbits = 0);
		void parseFieldsContinue(char*& tag_buffer, u32& cur_byte_bits_left, u32 nbits = 0);
		void clearFields();
	};
};