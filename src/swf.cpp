#include <cstdio>
#include <cstring>
#include <string>

#include <zlib.h>
#include <lzma.h>

#include <swf.hpp>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

using std::to_string;

using std::ifstream;
using std::ios_base;
using std::endl;

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
	
	char* SWFHeader::loadOtherData(char* swf_buffer)
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
		
		swf_buffer_i += 2;
		
		printf("\n");
		
		printf("SWF version: %d\n", version);
		printf("Decompressed file length: %d\n", file_length);
		
		printf("\n");
		
		printf("Window dimensions:\n");
		printf("xmin: %d twips\n", frame_size.xmin);
		printf("xmax: %d twips\n", frame_size.xmax);
		printf("ymin: %d twips\n", frame_size.ymin);
		printf("ymax: %d twips\n", frame_size.ymax);
		
		printf("\n");
		
		printf("Which means resolution is %dx%d\n", (frame_size.xmax - frame_size.xmin)/20, (frame_size.ymax - frame_size.ymin)/20);
		
		printf("\n");
		
		printf("FPS: %d\n", framerate >> 8);
		printf("SWF frame count: %d\n", frame_count);
		
		return swf_buffer + swf_buffer_i;
	}
	
	
	
	SWF::SWF()
	{
		
	}
	
	SWF::SWF(const char* swf_path) : next_frame_i(0), another_frame(false), next_script_i(0), last_queued_script(0)
	{
		// Configure reusable struct records
		// 
		// Using a SWFTag without parsing the header
		// behaves exactly like a SWF struct record
		RGB.setFieldCount(3);
		RGB.configureNextField(SWF_FIELD_UI8);  // Red
		RGB.configureNextField(SWF_FIELD_UI8);  // Green
		RGB.configureNextField(SWF_FIELD_UI8);  // Blue
		
		printf("Reading %s...\n", swf_path);
		
		ifstream swf_file(swf_path, ios_base::in | ios_base::binary);
		if (!swf_file.good())
		{
			EXC_ARG("SWF file `%s' not found\n", swf_path);
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
				
				printf("SWF is uncompressed.\n");
				
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
				
				printf("SWF is compressed with LZMA. Decompressing...\n");
				
				char* swf_buffer_uncompressed = new char[header.file_length];
				memcpy(swf_buffer_uncompressed, swf_buffer, 8);
				
				lzma_stream swf_lzma_stream = LZMA_STREAM_INIT;
				if (lzma_alone_decoder(&swf_lzma_stream, UINT64_MAX) != LZMA_OK)
				{
					EXC("Couldn't initialize LZMA decoding stream\n");
				}
				
				char lzma_header_swf[13];
				memcpy(&lzma_header_swf, &swf_buffer[12], 5);
				*((u64*) &lzma_header_swf[5]) = (u64) (header.file_length - 8);
				
				swf_lzma_stream.next_in = const_cast<const u8*>((u8*) lzma_header_swf);
				swf_lzma_stream.avail_in = 13;
				swf_lzma_stream.next_out = (u8*) &swf_buffer_uncompressed[8];
				swf_lzma_stream.avail_out = header.file_length - 8;
				
				lzma_ret ret = lzma_code(&swf_lzma_stream, LZMA_RUN);
				if (ret != LZMA_OK)
				{
					EXC_ARG("Couldn't successfully decode LZMA header, returned %d\n", ret);
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
				EXC("Invalid SWF compression format\n");
			}
		}
		
		cur_pos = header.loadOtherData(swf_buffer);
	}
	
	bool SWF::parseTag(ofstream& tag_main, const string& output_scripts_folder)
	{
		SWFTag tag;
		cur_pos = tag.parseHeader(cur_pos);
		
		interpretTag(tag, tag_main, output_scripts_folder);
		
		return tag.code != 0;
	}
	
	void SWF::parseAllTags(ofstream& tag_main, const string& output_scripts_folder)
	{
		SWFTag tag;
		tag.code = SWF_TAG_SHOW_FRAME;
		
		tag_main << "#include <recomp.h>" << endl
				 << "#include <out.h>" << endl << endl
				 << "void frame_" << to_string(next_frame_i) << "()" << endl
				 << "{" << endl;
		next_frame_i += 1;
		
		ofstream out_script_header(output_scripts_folder + "out.h", ios_base::out);
		out_script_header << "#pragma once" << endl;
		out_script_header.close();
		
		ofstream out_script_defs(output_scripts_folder + "script_defs.c", ios_base::out);
		out_script_defs << "#include \"script_decls.h\"" << endl;
		out_script_defs.close();
		
		ofstream out_script_decls(output_scripts_folder + "script_decls.h", ios_base::out);
		out_script_decls << "#pragma once" << endl << endl
						 << "#include <stackvalue.h>" << endl;
		out_script_decls.close();
		
		while (tag.code != 0)
		{
			cur_pos = tag.parseHeader(cur_pos);
			interpretTag(tag, tag_main, output_scripts_folder);
			tag.clearFields();
		}
		
		tag_main << endl << endl
				 << "typedef void (*frame_func)();" << endl << endl
				 << "frame_func frame_funcs[] =" << endl
				 << "{" << endl;
		
		for (size_t i = 0; i < next_frame_i; ++i)
		{
			tag_main << "\t" << "frame_" << to_string(i) << "," << endl;
		}
		
		tag_main << "};" << endl << endl
				 << "void tagMain()" << endl
				 << "{" << endl
				 << "\t" << "while (!quit_swf)" << endl
				 << "\t" << "{" << endl
				 << "\t\t" << "frame_funcs[next_frame]();" << endl
				 << "\t\t" << "if (!manual_next_frame)" << endl
				 << "\t\t" << "{" << endl
				 << "\t\t\t" << "next_frame += 1;" << endl
				 << "\t\t" << "}" << endl
				 << "\t\t" << "manual_next_frame = 0;" << endl
				 << "\t" << "}" << endl
				 << "}";
	}
	
	void SWF::interpretTag(SWFTag& tag, ofstream& tag_main, const string& output_scripts_folder)
	{
		printf("tag code: %d, tag length: %d\n", tag.code, tag.length);
		
		if (another_frame && tag.code != SWF_TAG_END_TAG)
		{
			tag_main << "}" << endl << endl
					 << "void frame_" << to_string(next_frame_i) << "()" << endl
					 << "{" << endl;
			next_frame_i += 1;
			
			another_frame = false;
		}
		
		switch (tag.code)
		{
			case SWF_TAG_END_TAG:
			{
				if (next_frame_i == 1)
				{
					tag_main << "\t" << "quit_swf = 1;" << endl;
				}
				
				else
				{
					tag_main << "\t" << "if (!manual_next_frame)" << endl
							 << "\t" << "{" << endl
							 << "\t\t" << "next_frame = 0;" << endl
							 << "\t\t" << "manual_next_frame = 1;" << endl
							 << "\t" << "}" << endl;
				}
				
				tag_main << "}";
				
				break;
			}
			
			case SWF_TAG_SHOW_FRAME:
			{
				while (last_queued_script < next_script_i)
				{
					tag_main << "\t" << "script_" << to_string(last_queued_script) << "(stack, &sp);" << endl;
					last_queued_script += 1;
				}
				
				tag_main << "\t" << "tagShowFrame();" << endl;
				
				another_frame = true;
				
				break;
			}
			
			case SWF_TAG_SET_BACKGROUND_COLOR:
			{
				cur_pos = RGB.parseFields(cur_pos);
				
				tag_main << "\t" << "tagSetBackgroundColor("
						 << to_string((u8) RGB.fields[0].value) << ", "
						 << to_string((u8) RGB.fields[1].value) << ", "
						 << to_string((u8) RGB.fields[2].value) << ");" << endl;
				
				break;
			}
			
			case SWF_TAG_DO_ACTION:
			{
				ofstream out_script_header(output_scripts_folder + "out.h", ios_base::app);
				ofstream out_script_defs(output_scripts_folder + "script_defs.c", ios_base::app);
				ofstream out_script_decls(output_scripts_folder + "script_decls.h", ios_base::app);
				
				out_script_header << endl << "void script_" << to_string(next_script_i) << "(char* stack, u32* sp);";
				
				ofstream out_script(output_scripts_folder + "script_" + to_string(next_script_i) + ".c", ios_base::out);
				
				out_script << "#include <recomp.h>" << endl
						   << "#include \"script_decls.h\"" << endl << endl
						   << "u32 oldSP;" << endl << endl
						   << "void script_" << next_script_i << "(char* stack, u32* sp)" << endl
						   << "{" << endl;
				next_script_i += 1;
				
				cur_pos = action.parseActions(cur_pos, out_script, out_script_defs, out_script_decls);
				
				out_script << "}";
				
				break;
			}
			
			case SWF_TAG_ENABLE_DEBUGGER:
			{
				cur_pos += tag.length;
				
				break;
			}
			
			case SWF_TAG_ENABLE_DEBUGGER_2:
			{
				cur_pos += tag.length;
				
				break;
			}
			
			case SWF_TAG_SCRIPT_LIMITS:
			{
				tag.setFieldCount(2);
				tag.configureNextField(SWF_FIELD_UI16);
				tag.configureNextField(SWF_FIELD_UI16);
				
				cur_pos = tag.parseFields(cur_pos);
				
				tag_main << "\t" << "tagScriptLimits("
						 << to_string((u16) tag.fields[0].value) << ", "
						 << to_string((u16) tag.fields[1].value) << ");" << endl;
				
				break;
			}
			
			case SWF_TAG_FILE_ATTRIBUTES:
			{
				tag.setFieldCount(4);
				
				tag.configureNextField(SWF_FIELD_UI8);
				tag.configureNextField(SWF_FIELD_UI8);
				tag.configureNextField(SWF_FIELD_UI8);
				tag.configureNextField(SWF_FIELD_UI8);
				
				cur_pos = tag.parseFields(cur_pos);
				
				u8 flags = (u8) tag.fields[0].value;
				
				if ((flags & 0b00001000) != 0)
				{
					//EXC("ActionScript 3 SWFs not implemented.\n");
				}
				
				break;
			}
			
			case SWF_TAG_SYMBOL_CLASS:
			{
				cur_pos += tag.length;
				
				break;
			}
			
			case SWF_TAG_METADATA:
			{
				cur_pos += tag.length;
				
				break;
			}
			
			//~ case SWF_TAG_DO_ABC:
			//~ {
				//~ cur_pos += tag.length;
				
				//~ break;
			//~ }
			
			default:
			{
				EXC_ARG("Tag type %d not implemented.\n", tag.code);
			}
		}
	}
};