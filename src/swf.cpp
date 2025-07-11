#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <zlib.h>
#include <lzma.h>

#include <swf.hpp>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

#define VAL(type, x) (*((type*) x))

#define CROSS(v1, v2) (v1.x*v2.y - v2.x*v1.y)

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
	
	void SWFHeader::loadOtherData(char*& swf_buffer)
	{
		SWFTag rect;
		
		rect.setFieldCount(5);
		
		rect.configureNextField(SWF_FIELD_UB, 5, true);
		rect.configureNextField(SWF_FIELD_SB, 0);
		rect.configureNextField(SWF_FIELD_SB, 0);
		rect.configureNextField(SWF_FIELD_SB, 0);
		rect.configureNextField(SWF_FIELD_SB, 0);
		
		rect.parseFields(swf_buffer);
		
		frame_size.nbits = (u8) rect.fields[0].value;
		frame_size.xmin = (s32) rect.fields[1].value;
		frame_size.xmax = (s32) rect.fields[2].value;
		frame_size.ymin = (s32) rect.fields[3].value;
		frame_size.ymax = (s32) rect.fields[4].value;
		
		framerate = *((u16*) swf_buffer);
		swf_buffer += 2;
		
		frame_count = *((u16*) swf_buffer);
		swf_buffer += 2;
		
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
		
		cur_pos = swf_buffer + 8;
		
		header.loadOtherData(cur_pos);
	}
	
	void SWF::parseAllTags(ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header, const string& output_scripts_folder)
	{
		SWFTag tag;
		tag.code = SWF_TAG_SHOW_FRAME;
		
		tag_main << "#include <recomp.h>" << endl << endl
				 << "#include <out.h>" << endl
				 << "#include \"draws.h\"" << endl << endl
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
			tag.parseHeader(cur_pos);
			interpretTag(tag, tag_main, out_draws, out_draws_header, output_scripts_folder);
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
		
		tag_main << "};";
	}
	
	void SWF::interpretTag(SWFTag& tag, ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header, const string& output_scripts_folder)
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
			
			case SWF_TAG_DEFINE_SHAPE:
			{
				interpretShape(tag, tag_main, out_draws, out_draws_header);
				
				break;
			}
			
			case SWF_TAG_SET_BACKGROUND_COLOR:
			{
				RGB.parseFields(cur_pos);
				
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
						   << "void script_" << next_script_i << "(char* stack, u32* sp)" << endl
						   << "{" << endl;
				next_script_i += 1;
				
				action.parseActions(cur_pos, out_script, out_script_defs, out_script_decls);
				
				out_script << "}";
				
				break;
			}
			
			case SWF_TAG_PLACE_OBJECT_2:
			{
				tag.setFieldCount(2);
				
				tag.configureNextField(SWF_FIELD_UI8);
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				u16 depth = (u16) tag.fields[1].value;
				
				// TODO: check flags to dynamically configure next fields
				
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				u16 char_id = (u16) tag.fields[0].value;
				
				u32 cur_byte_bits_left = 8;
				
				// configure MATRIX record
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UB, 1);
				
				tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
				
				bool has_scale = tag.fields[0].value & 1;
				
				if (has_scale)
				{
					tag.clearFields();
					tag.setFieldCount(3);
					
					tag.configureNextField(SWF_FIELD_UB, 5, true);
					tag.configureNextField(SWF_FIELD_FB, 0);
					tag.configureNextField(SWF_FIELD_FB, 0);
					
					tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
					
					u32 nscale_bits = (u32) tag.fields[0].value;
					float scale_x = VAL(float, &tag.fields[1].value);
					float scale_y = VAL(float, &tag.fields[2].value);
				}
				
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UB, 1);
				
				tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
				
				bool has_rotate = tag.fields[0].value & 1;
				
				if (has_rotate)
				{
					tag.clearFields();
					tag.setFieldCount(3);
					
					tag.configureNextField(SWF_FIELD_UB, 5, true);
					tag.configureNextField(SWF_FIELD_FB, 0);
					tag.configureNextField(SWF_FIELD_FB, 0);
					
					tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
					
					u32 nrotate_bits = (u32) tag.fields[0].value;
					float rotateskew_0 = VAL(float, &tag.fields[1].value);
					float rotateskew_1 = VAL(float, &tag.fields[2].value);
				}
				
				tag.clearFields();
				tag.setFieldCount(3);
				
				tag.configureNextField(SWF_FIELD_UB, 5, true);
				tag.configureNextField(SWF_FIELD_FB, 0);
				tag.configureNextField(SWF_FIELD_FB, 0);
				
				tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
				
				u32 ntranslate_bits = (u32) tag.fields[0].value;
				s32 translate_x = (u32) tag.fields[1].value;
				s32 translate_y = (u32) tag.fields[2].value;
				
				if (cur_byte_bits_left != 8)
				{
					cur_pos += 1;
				}
				
				tag_main << "\t" << "tagPlaceObject2(" << to_string(depth) << ", " << to_string(char_id) << ");" << endl;
				
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
				
				tag.parseFields(cur_pos);
				
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
				
				tag.parseFields(cur_pos);
				
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
	
	void SWF::interpretShape(SWFTag& shape_tag, ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header)
	{
		switch (shape_tag.code)
		{
			case SWF_TAG_DEFINE_SHAPE:
			{
				shape_tag.setFieldCount(6);
				
				shape_tag.configureNextField(SWF_FIELD_UI16, 16);
				shape_tag.configureNextField(SWF_FIELD_UB, 5, true);
				shape_tag.configureNextField(SWF_FIELD_SB, 0);
				shape_tag.configureNextField(SWF_FIELD_SB, 0);
				shape_tag.configureNextField(SWF_FIELD_SB, 0);
				shape_tag.configureNextField(SWF_FIELD_SB, 0);
				
				shape_tag.parseFields(cur_pos);
				
				u16 shape_id = (u16) shape_tag.fields[0].value;
				
				// FILLSTYLEARRAY
				shape_tag.clearFields();
				shape_tag.setFieldCount(1);
				
				shape_tag.configureNextField(SWF_FIELD_UI8, 8);
				
				shape_tag.parseFields(cur_pos);
				
				u16 fill_style_count = (u8) shape_tag.fields[0].value;
				
				if (fill_style_count == 0xFF)
				{
					shape_tag.clearFields();
					
					shape_tag.configureNextField(SWF_FIELD_UI16, 16);
					
					shape_tag.parseFields(cur_pos);
					
					fill_style_count = (u16) shape_tag.fields[0].value;
				}
				
				for (u16 i = 0; i < fill_style_count; ++i)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(4);
					
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
				}
				
				// LINESTYLEARRAY
				shape_tag.clearFields();
				shape_tag.setFieldCount(1);
				
				shape_tag.configureNextField(SWF_FIELD_UI8, 8);
				
				shape_tag.parseFields(cur_pos);
				
				u16 line_style_count = (u8) shape_tag.fields[0].value;
				
				if (line_style_count == 0xFF)
				{
					shape_tag.clearFields();
					
					shape_tag.configureNextField(SWF_FIELD_UI16, 16);
					
					shape_tag.parseFields(cur_pos);
					
					line_style_count = (u16) shape_tag.fields[0].value;
				}
				
				for (u16 i = 0; i < line_style_count; ++i)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(4);
					
					shape_tag.configureNextField(SWF_FIELD_UI16, 16);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
				}
				
				shape_tag.clearFields();
				shape_tag.setFieldCount(2);
				
				shape_tag.configureNextField(SWF_FIELD_UB, 4);
				shape_tag.configureNextField(SWF_FIELD_UB, 4);
				
				shape_tag.parseFields(cur_pos);
				
				u8 fill_bits = (u8) shape_tag.fields[0].value;
				u8 line_bits = (u8) shape_tag.fields[1].value;
				
				u32 last_fill_style_0 = 0;
				u32 last_fill_style_1 = 0;
				
				std::vector<std::vector<Vertex>> shapes;
				shapes.push_back(std::vector<Vertex>());
				
				s32 last_x = 0;
				s32 last_y = 0;
				
				u32 cur_byte_bits_left = 8;
				
				while (true)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(2);
					
					shape_tag.configureNextField(SWF_FIELD_UB, 1);
					shape_tag.configureNextField(SWF_FIELD_UB, 5);
					
					shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
					
					bool is_edge_record = (u8) shape_tag.fields[0].value;
					u8 state_flags = (u8) shape_tag.fields[1].value;
					
					if (is_edge_record)
					{
						bool is_straight_edge = (state_flags & 0b10000) != 0;
						u8 num_bits = (u8) state_flags & 0xF;
						
						if (is_straight_edge)
						{
							// StraightEdgeRecord
							
							shape_tag.clearFields();
							shape_tag.setFieldCount(1);
							
							shape_tag.configureNextField(SWF_FIELD_UB, 1);
							
							shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
							
							bool is_general_line = (shape_tag.fields[0].value & 1) != 0;
							
							if (is_general_line)
							{
								shape_tag.clearFields();
								shape_tag.setFieldCount(2);
								
								shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
								shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
								
								shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
								
								s16 delta_x = (s16) shape_tag.fields[0].value;
								s16 delta_y = (s16) shape_tag.fields[1].value;
								
								Vertex v;
								v.x = last_x + (s32) delta_x;
								v.y = last_y + (s32) delta_y;
								
								shapes[0].push_back(v);
								
								last_x = v.x;
								last_y = v.y;
								
								continue;
							}
							
							shape_tag.clearFields();
							shape_tag.setFieldCount(2);
							
							shape_tag.configureNextField(SWF_FIELD_UB, 1);
							shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
							
							shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
							
							bool is_vertical_line = (shape_tag.fields[0].value & 1) != 0;
							s16 delta = (s16) shape_tag.fields[1].value;
							
							Vertex v;
							
							v.x = last_x;
							v.y = last_y;
							
							if (is_vertical_line)
							{
								v.y += (s32) delta;
							}
							
							else
							{
								v.x += (s32) delta;
							}
							
							shapes[0].push_back(v);
							
							last_x = v.x;
							last_y = v.y;
							
							continue;
						}
						
						// CurvedEdgeRecord
						
						shape_tag.clearFields();
						shape_tag.setFieldCount(4);
						
						shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
						shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
						shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
						shape_tag.configureNextField(SWF_FIELD_SB, num_bits + 2);
						
						shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
						
						s16 control_delta_x = (s16) shape_tag.fields[0].value;
						s16 control_delta_y = (s16) shape_tag.fields[1].value;
						s16 anchor_delta_x = (s16) shape_tag.fields[2].value;
						s16 anchor_delta_y = (s16) shape_tag.fields[3].value;
						
						continue;
					}
					
					if (state_flags == 0)
					{
						// EndShapeRecord
						break;
					}
					
					// StyleChangeRecord
					
					// StateNewStyles is only used by DefineShape2 and DefineShape3
					bool state_new_styles = (state_flags & 0b10000) != 0;
					bool state_line_style = (state_flags & 0b01000) != 0;
					bool state_fill_style_1 = (state_flags & 0b00100) != 0;
					bool state_fill_style_0 = (state_flags & 0b00010) != 0;
					bool state_move_to = (state_flags & 0b00001) != 0;
					
					shape_tag.clearFields();
					shape_tag.setFieldCount(3*state_move_to + state_fill_style_0 + state_fill_style_1 + state_line_style);
					
					if (state_move_to)
					{
						shape_tag.configureNextField(SWF_FIELD_UB, 5, true);
						shape_tag.configureNextField(SWF_FIELD_SB, 0);
						shape_tag.configureNextField(SWF_FIELD_SB, 0);
					}
					
					if (state_fill_style_0)
					{
						shape_tag.configureNextField(SWF_FIELD_UB, fill_bits);
					}
					
					if (state_fill_style_1)
					{
						shape_tag.configureNextField(SWF_FIELD_UB, fill_bits);
					}
					
					if (state_line_style)
					{
						shape_tag.configureNextField(SWF_FIELD_UB, line_bits);
					}
					
					shape_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
					
					u8 move_bits;
					u32 move_delta_x;
					u32 move_delta_y;
					
					u32 fill_style_0;
					u32 fill_style_1;
					
					u32 line_style;
					
					size_t current_field = 0;
					
					if (state_move_to)
					{
						move_bits = (u8) shape_tag.fields[current_field++].value;
						move_delta_x = (u32) shape_tag.fields[current_field++].value;
						move_delta_y = (u32) shape_tag.fields[current_field++].value;
						
						Vertex v;
						v.x = last_x + move_delta_x;
						v.y = last_y + move_delta_y;
						
						shapes[0].push_back(v);
						
						last_x = v.x;
						last_y = v.y;
						
						fprintf(stderr, "move bits: %d, delta x: %d, delta y: %d\n", move_bits, move_delta_x, move_delta_y);
					}
					
					if (state_fill_style_0)
					{
						fill_style_0 = (u32) shape_tag.fields[current_field++].value;
						
						//~ if (fill_style_0 != 0 && fill_style_0 != last_fill_style_0)
						//~ {
							
						//~ }
						
						fprintf(stderr, "fill style 0: %d\n", fill_style_0);
					}
					
					if (state_fill_style_1)
					{
						fill_style_1 = (u32) shape_tag.fields[current_field++].value;
						
						fprintf(stderr, "fill style 1: %d\n", fill_style_1);
					}
					
					if (state_line_style)
					{
						line_style = (u32) shape_tag.fields[current_field++].value;
						
						fprintf(stderr, "line style: %d\n", line_style);
					}
				}
				
				if (cur_byte_bits_left != 8)
				{
					cur_pos += 1;
				}
				
				for (size_t i = 0; i < shapes[0].size(); ++i)
				{
					fprintf(stderr, "got x: %d, y: %d\n", shapes[0][i].x, shapes[0][i].y);
				}
				
				std::vector<Tri> tris;
				
				fillShape(shapes[0], tris);
				
				std::string shape_name = "shape_" + to_string(shape_id) + "_tris";
				
				out_draws << endl;
				
				out_draws << "float " << shape_name << "[" << to_string(3*tris.size()) << "][7] =" << endl
						  << "{" << endl;
				
				for (Tri t : tris)
				{
					for (int i = 0; i < 3; ++i)
					{
						out_draws << "\t" << "{ ";
						out_draws << to_string(t.verts[i].x) << "/" << "5500.0f - 1.0f, ";
						out_draws << "1.0f - " << to_string(t.verts[i].y) << "/" << "4000.0f, ";
						out_draws << "0.0f, ";
						out_draws << "1.0f, ";
						out_draws << "0.0f, ";
						out_draws << "0.0f, ";
						out_draws << "1.0f }," << endl;
					}
				}
				
				out_draws << "};" << endl;
				
				out_draws_header << endl << "extern float " << shape_name << "[" << to_string(3*tris.size()) << "][7];" << endl;
				
				tag_main << "\t" << "dictionary[" << to_string(shape_id) << "] = (char*) " << shape_name << ";" << endl;
				tag_main << "\t" << "dictionary_sizes[" << to_string(shape_id) << "] = sizeof(" << shape_name << ");" << endl;
				
				break;
			}
		}
	}
	
	void SWF::fillShape(const std::vector<Vertex>& shape, std::vector<Tri>& tris)
	{
		size_t i = 0;
		
		Tri t;
		
		for (Vertex v : shape)
		{
			if (i == 3)
			{
				break;
			}
			
			t.verts[i] = v;
			i += 1;
		}
		
		tris.push_back(t);
	}
};