#include <cstring>
#include <cstdio>

#include <swf.h>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

namespace SWFRecomp
{
	SWFHeader::SWFHeader()
	{
		
	}
	
	SWFHeader::SWFHeader(char* swf_data)
	{
		// I *will* absolutely litter this with comments
		// because this format is reprehensible to parse
		
		// Get the initial data (SWF is always little-endian)
		memcpy(this, swf_data, 8);
		
		// (I wish they put the RECT at the *end* of the header)
		// We have to know how large the RECT struct is before continuing
		frame_size.nbits = (swf_data[8] >> 3) & 0b11111;
		
		// Start off with the first byte
		u8 swf_data_i = 8;
		u8 cur_byte_bits_left = 3;
		
		// Now prepare to read in the RECT
		// Temporary form of RECT's member variables
		u32 rect_u32s[4];
		memset(rect_u32s, 0, 16);
		
		// Which member variable are we reading?
		u8 cur_rect_var_i = 0;
		
		// How many bits does it have left to read?
		u8 cur_rect_var_bits_left = frame_size.nbits;
		
		while (cur_rect_var_i < 4)
		{
			u8 bits_left = MIN(cur_rect_var_bits_left, cur_byte_bits_left);
			u8 mask = (((s8) 0x80) >> (bits_left - 1));
			
			u8 temp_rect_byte = swf_data[swf_data_i] << (8 - cur_byte_bits_left);
			
			rect_u32s[cur_rect_var_i] <<= bits_left;
			rect_u32s[cur_rect_var_i] |= (temp_rect_byte & mask) >> 8 - bits_left;
			
			cur_rect_var_bits_left -= bits_left;
			cur_byte_bits_left -= bits_left;
			
			if (cur_byte_bits_left == 0)
			{
				swf_data_i += 1;
				cur_byte_bits_left = 8;
			}
			
			if (cur_rect_var_bits_left == 0)
			{
				cur_rect_var_i += 1;
				cur_rect_var_bits_left = frame_size.nbits;
			}
		}
		
		// You wanna know something else?
		// Half of those values aren't used. :p
		
		// Skip to the next byte if we haven't already
		if (cur_byte_bits_left != 8)
		{
			swf_data_i += 1;
		}
		
		// Careful, this value is fixed-point, so it behaves like big-endian
		framerate = (swf_data[swf_data_i] << 8) | swf_data[swf_data_i + 1];
		
		swf_data_i += 2;
		frame_count = *((u16*) &swf_data[swf_data_i]);
	}
};