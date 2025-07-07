#include <cstdio>
#include <exception>

#include <field.hpp>

#define VAL(type, x) ((u64) *((type*) x))
#define MIN(x, y) ((x < y) ? x : y)

namespace SWFRecomp
{
	SWFField::SWFField()
	{
		bit_length = 0;
		type = SWF_FIELD_NONE;
		value = 0;
		is_nbits = false;
	}
	
	void SWFField::parse(char*& field_buffer, u32& nbits, u32& cur_byte_bits_left, bool& prev_was_bitfield)
	{
		bool current_is_bitfield = (type == SWF_FIELD_UB ||
								    type == SWF_FIELD_SB ||
								    type == SWF_FIELD_FB);
		
		if (prev_was_bitfield && !current_is_bitfield && cur_byte_bits_left != 8)
		{
			field_buffer += 1;
			cur_byte_bits_left = 8;
		}
		
		switch (type)
		{
			case SWF_FIELD_NONE:
			{
				EXC("No type set for field.\n");
			}
			
			case SWF_FIELD_SI8:
			{
				value = VAL(s8, field_buffer);
				field_buffer += 1;
				
				break;
			}
			
			case SWF_FIELD_SI16:
			{
				value = VAL(s16, field_buffer);
				field_buffer += 2;
				
				break;
			}
			
			case SWF_FIELD_SI32:
			{
				value = VAL(s32, field_buffer);
				field_buffer += 4;
				
				break;
			}
			
			case SWF_FIELD_UI8:
			{
				value = VAL(u8, field_buffer);
				field_buffer += 1;
				
				break;
			}
			
			case SWF_FIELD_UI16:
			{
				value = VAL(u16, field_buffer);
				field_buffer += 2;
				
				break;
			}
			
			case SWF_FIELD_UI32:
			{
				value = VAL(u32, field_buffer);
				field_buffer += 4;
				
				break;
			}
			
			case SWF_FIELD_UB:
			{
				parseBitField(field_buffer, (bit_length == 0) ? nbits : bit_length, cur_byte_bits_left, false);
				
				if (is_nbits)
				{
					nbits = (u32) value;
				}
				
				prev_was_bitfield = true;
				
				break;
			}
			
			case SWF_FIELD_SB:
			{
				u8 length = (bit_length == 0) ? nbits : bit_length;
				
				parseBitField(field_buffer, length, cur_byte_bits_left, true);
				
				int shiftAmount = 64 - length;
				
				s64 temp = (s64) (value << shiftAmount);
				value = temp >> shiftAmount;
				
				if (is_nbits)
				{
					nbits = (u32) value;
				}
				
				prev_was_bitfield = true;
				
				break;
			}
			
			default:
			{
				EXC_ARG("Field type %d not implemented.\n", type);
			}
		}
		
		prev_was_bitfield = current_is_bitfield;
	}
	
	void SWFField::parseBitField(char*& field_buffer, u32 nbits, u32& cur_byte_bits_left, bool sb)
	{
		// How many bits does it have left to read?
		u8 cur_field_bits_left = nbits;
		
		while (cur_field_bits_left > 0)
		{
			u8 bits_left = MIN(cur_field_bits_left, cur_byte_bits_left);
			u8 mask = (((s8) 0x80) >> (bits_left - 1));
			
			u8 temp_field_byte = *field_buffer << (8 - cur_byte_bits_left);
			
			value <<= bits_left;
			
			if (sb)
			{
				value |= (temp_field_byte & mask) >> (8 - bits_left);
			}
			
			else
			{
				value |= (temp_field_byte >> (8 - bits_left)) & (mask >> (8 - bits_left));
			}
			
			cur_field_bits_left -= bits_left;
			cur_byte_bits_left -= bits_left;
			
			if (cur_byte_bits_left == 0)
			{
				field_buffer += 1;
				cur_byte_bits_left = 8;
			}
			
			if (cur_field_bits_left == 0)
			{
				break;
			}
		}
	}
};