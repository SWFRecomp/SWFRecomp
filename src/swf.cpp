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
		
		printf("\n");
		
		printf("SWF version: %d\n", version);
		
		printf("\n");
		
		printf("Window dimensions:\n");
		printf("xmin: %d twips\n", frame_size.xmin);
		printf("xmax: %d twips\n", frame_size.xmax);
		printf("ymin: %d twips\n", frame_size.ymin);
		printf("ymax: %d twips\n", frame_size.ymax);
		
		printf("\n");
		
		printf("FPS: %d\n", framerate >> 8);
		printf("SWF frame count: %d\n", frame_count);
		
		return true;
	}
	
	
	
	SWF::SWF()
	{
		
	}
	
	SWF::SWF(const char* swf_path)
	{
		printf("Reading %s...\n", swf_path);
		
		ifstream swf_file(swf_path, ios_base::in | ios_base::binary);
		if (!swf_file.good())
		{
			fprintf(stderr, "SWF file `%s' not found\n", swf_path);
			throw std::exception();
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
				
				printf("SWF is compressed with zlib. Decompressing...\n");
				
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
				// Yeah, Adobe definitely screwed the format up on this one.
				// I'm not sure if they just didn't get it, or what...
				// Whatever this mangled garbage is, it's NOT REAL LZMA.
				
				char* swf_buffer_uncompressed = new char[header.file_length];
				memcpy(swf_buffer_uncompressed, swf_buffer, 8);
				
				lzma_stream swf_lzma_stream = LZMA_STREAM_INIT;
				if (lzma_alone_decoder(&swf_lzma_stream, UINT64_MAX) != LZMA_OK)
				{
					fprintf(stderr, "Couldn't initialize LZMA decoding stream\n");
					throw std::exception();
				}
				
				char lzma_header_swf[13];
				memset(lzma_header_swf, 0, 13);
				memcpy(&lzma_header_swf, &swf_buffer[12], 5);
				*((u64*) &lzma_header_swf[5]) = (u64) (header.file_length - 8);
				
				swf_lzma_stream.next_in = const_cast<const u8*>((u8*) lzma_header_swf);
				swf_lzma_stream.avail_in = 13;
				swf_lzma_stream.next_out = (u8*) &swf_buffer_uncompressed[8];
				swf_lzma_stream.avail_out = header.file_length - 8;
				
				lzma_ret ret = lzma_code(&swf_lzma_stream, LZMA_RUN);
				if (ret != LZMA_OK)
				{
					fprintf(stderr, "Couldn't successfully decode LZMA header, returned %d\n", ret);
					throw std::exception();
				}
				
				swf_lzma_stream.next_in = const_cast<const u8*>((u8*) &swf_buffer[8 + 4 + 5]);
				swf_lzma_stream.avail_in = swf_size - 8 - 4 - 5;
				
				ret = lzma_code(&swf_lzma_stream, LZMA_FINISH);
				
				lzma_end(&swf_lzma_stream);
				
				delete[] swf_buffer;
				swf_buffer = swf_buffer_uncompressed;
				
				break;
			}
			
			default:
			{
				fprintf(stderr, "Invalid SWF compression format\n");
				throw std::exception();
				
				break;
			}
		}
		
		header.load_other_data(swf_buffer);
	}
};