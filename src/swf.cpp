#include <cstdio>
#include <cstring>
#include <fstream>

#include <zlib.h>
#include <lzma.h>

#include <swf.h>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

using std::ifstream;
using std::ios_base;

namespace SWFRecomp
{
	SWFHeader::SWFHeader()
	{
		
	}
	
	SWFHeader::SWFHeader(char* swf_buffer)
	{
		// Get the initial data (SWF is always little-endian)
		memcpy(this, swf_buffer, 8);
	}
	
	int SWFHeader::load_other_data(char* swf_buffer)
	{
		// I *will* absolutely litter this with comments
		// because this format is reprehensible to parse
		
		// (I wish they put the RECT at the *end* of the header)
		// We have to know how large the RECT struct is before continuing
		frame_size.nbits = (swf_buffer[8] >> 3) & 0b11111;
		
		// Start off with the first byte
		u8 swf_buffer_i = 8;
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
			
			u8 temp_rect_byte = swf_buffer[swf_buffer_i] << (8 - cur_byte_bits_left);
			
			rect_u32s[cur_rect_var_i] <<= bits_left;
			rect_u32s[cur_rect_var_i] |= (temp_rect_byte & mask) >> (8 - bits_left);
			
			cur_rect_var_bits_left -= bits_left;
			cur_byte_bits_left -= bits_left;
			
			if (cur_byte_bits_left == 0)
			{
				swf_buffer_i += 1;
				cur_byte_bits_left = 8;
			}
			
			if (cur_rect_var_bits_left == 0)
			{
				cur_rect_var_i += 1;
				cur_rect_var_bits_left = frame_size.nbits;
			}
		}
		
		frame_size.xmin = (s32) rect_u32s[0];
		frame_size.xmax = (s32) rect_u32s[1];
		frame_size.ymin = (s32) rect_u32s[2];
		frame_size.ymax = (s32) rect_u32s[3];
		
		// Skip to the next byte if we haven't already
		if (cur_byte_bits_left != 8)
		{
			swf_buffer_i += 1;
		}
		
		framerate = *((u16*) &swf_buffer[swf_buffer_i]);
		
		swf_buffer_i += 2;
		frame_count = *((u16*) &swf_buffer[swf_buffer_i]);
		
		return true;
	}
	
	
	
	SWF::SWF()
	{
		
	}
	
	SWF::SWF(const char* swf_path)
	{
		ifstream swf_file(swf_path, ios_base::in | ios_base::binary);
		if (!swf_file.good())
		{
			fprintf(stderr, "SWF file `%s' not found\n", swf_path);
			throw new std::exception();
		}
		
		swf_file.seekg(0, ios_base::end);
		size_t swf_size = swf_file.tellg();
		swf_file.seekg(0, ios_base::beg);
		
		swf_buffer = new char[swf_size];
		swf_file.read(swf_buffer, swf_size);
		swf_file.close();
		
		header = SWFHeader(swf_buffer);
		
		switch (header.compression)
		{
			case 'F':
			{
				// uncompressed
				
				break;
			}
			
			case 'C':
			{
				// zlib
				
				char* swf_buffer_uncompressed = new char[header.file_length];
				memcpy(swf_buffer_uncompressed, swf_buffer, 8);
				long unsigned int swf_length_no_8 = header.file_length - 8;
				uncompress((u8*) &swf_buffer_uncompressed[8], &swf_length_no_8, const_cast<const u8*>((u8*) &swf_buffer[8]), (uLong) (swf_size - 8));
				
				delete[] swf_buffer;
				swf_buffer = swf_buffer_uncompressed;
				
				break;
			}
			
			case 'Z':
			{
				// lzma
				
				fprintf(stderr, "WARNING: LZMA compression not fully tested yet\n");
				
				char* swf_buffer_uncompressed = new char[header.file_length];
				memcpy(swf_buffer_uncompressed, swf_buffer, 8);
				
				lzma_stream swf_lzma_stream = LZMA_STREAM_INIT;
				if (lzma_auto_decoder(&swf_lzma_stream, header.file_length, 0) != LZMA_OK)
				{
					fprintf(stderr, "Couldn't initialize LZMA decoding stream\n");
					throw new std::exception();
				}
				
				swf_lzma_stream.next_in = const_cast<const u8*>((u8*) &swf_buffer[8]);
				swf_lzma_stream.avail_in = swf_size - 8;
				swf_lzma_stream.next_out = (u8*) &swf_buffer_uncompressed[8];
				swf_lzma_stream.avail_out = header.file_length - 8;
				
				if (lzma_code(&swf_lzma_stream, LZMA_RUN) != LZMA_OK)
				{
					fprintf(stderr, "Couldn't successfully decode LZMA\n");
					throw new std::exception();
				}
				
				lzma_end(&swf_lzma_stream);
				
				break;
			}
			
			default:
			{
				fprintf(stderr, "Invalid SWF compression format\n");
				throw new std::exception();
				
				break;
			}
		}
		
		header.load_other_data(swf_buffer);
	}
};