#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#include <zlib.h>
#include <lzma.h>

#include <swf.hpp>

#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)

#define VAL(type, x) (*((type*) x))

#define CROSS(v1, v2) (v1.x*v2.y - v2.x*v1.y)

#define NOT_SHARED_LINKS(path1, path2) (std::find(path1.next_neighbors_forward.begin(), path1.next_neighbors_forward.end(), &path2) == path1.next_neighbors_forward.end() && \
										std::find(path2.next_neighbors_forward.begin(), path2.next_neighbors_forward.end(), &path1) == path2.next_neighbors_forward.end() && \
										std::find(path1.next_neighbors_backward.begin(), path1.next_neighbors_backward.end(), &path2) == path1.next_neighbors_backward.end() && \
										std::find(path2.next_neighbors_backward.begin(), path2.next_neighbors_backward.end(), &path1) == path2.next_neighbors_backward.end())

#define FRAME_WIDTH (header.frame_size.xmax - header.frame_size.xmin)
#define FRAME_HEIGHT (header.frame_size.ymax - header.frame_size.ymin)

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
				
				FillStyle* fill_styles = new FillStyle[fill_style_count];
				
				for (u16 i = 0; i < fill_style_count; ++i)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(4);
					
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
					
					fill_styles[i].type = (u8) shape_tag.fields[0].value;
					
					fill_styles[i].r = (u8) shape_tag.fields[1].value;
					fill_styles[i].g = (u8) shape_tag.fields[2].value;
					fill_styles[i].b = (u8) shape_tag.fields[3].value;
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
				
				std::vector<Path> paths;
				paths.reserve(512);
				
				Path* current_path = nullptr;
				
				s32 last_x = 0;
				s32 last_y = FRAME_HEIGHT;
				
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
								v.y = last_y - (s32) delta_y;
								
								current_path->verts.push_back(v);
								
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
								v.y -= (s32) delta;
							}
							
							else
							{
								v.x += (s32) delta;
							}
							
							current_path->verts.push_back(v);
							
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
					
					u32 fill_style_0 = last_fill_style_0;
					u32 fill_style_1 = last_fill_style_1;
					
					u32 line_style;
					
					bool fill_style_0_change = false;
					bool fill_style_1_change = false;
					
					size_t current_field = 0;
					
					if (state_move_to)
					{
						move_bits = (u8) shape_tag.fields[current_field++].value;
						move_delta_x = (u32) shape_tag.fields[current_field++].value;
						move_delta_y = (u32) shape_tag.fields[current_field++].value;
						
						last_x = move_delta_x;
						last_y = FRAME_HEIGHT - move_delta_y;
					}
					
					if (state_fill_style_0)
					{
						fill_style_0 = (u32) shape_tag.fields[current_field++].value;
						
						fill_style_0_change = fill_style_0 != last_fill_style_0;
					}
					
					if (state_fill_style_1)
					{
						fill_style_1 = (u32) shape_tag.fields[current_field++].value;
						
						fill_style_1_change = fill_style_1 != last_fill_style_1;
					}
					
					if (state_line_style)
					{
						line_style = (u32) shape_tag.fields[current_field++].value;
						
						fprintf(stderr, "line style: %d\n", line_style);
					}
					
					if (state_move_to || fill_style_0_change || fill_style_1_change)
					{
						paths.push_back(Path());
						current_path = &paths.back();
						
						current_path->verts.reserve(512);
						current_path->fill_styles[0] = fill_style_0;
						current_path->fill_styles[1] = fill_style_1;
						current_path->used = false;
						current_path->self_closed = false;
						
						Vertex v;
						v.x = last_x;
						v.y = last_y;
						
						current_path->verts.push_back(v);
					}
					
					last_fill_style_0 = fill_style_0;
					last_fill_style_1 = fill_style_1;
				}
				
				if (cur_byte_bits_left != 8)
				{
					cur_pos += 1;
				}
				
				std::vector<Path> paths_copy = paths;
				
				std::vector<Shape> shapes;
				
				shapes.push_back(Shape());
				
				bool changed = false;
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					if (paths[i].fill_styles[0] != 0 || paths[i].fill_styles[1] != 0)
					{
						fprintf(stderr, "path %zu with fill %d and %d\n", i, paths[i].fill_styles[0], paths[i].fill_styles[1]);
						for (size_t j = 0; j < paths[i].verts.size(); ++j)
						{
							fprintf(stderr, "has (%d, %d)\n", paths[i].verts[j].x / 20, (FRAME_HEIGHT - paths[i].verts[j].y) / 20);
						}
					}
				}
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					if (paths[i].fill_styles[0] != 0 || paths[i].fill_styles[1] != 0)
					{
						Vertex path_start;
						path_start.x = paths[i].verts[0].x;
						path_start.y = paths[i].verts[0].y;
						
						Vertex path_end;
						path_end.x = paths[i].verts.back().x;
						path_end.y = paths[i].verts.back().y;
						
						if (path_start.x == path_end.x &&
							path_start.y == path_end.y)
						{
							paths[i].self_closed = true;
							continue;
						}
						
						for (size_t j = 0; j < paths.size(); ++j)
						{
							if (i == j)
							{
								continue;
							}
							
							Vertex this_path_start;
							this_path_start.x = paths[j].verts[0].x;
							this_path_start.y = paths[j].verts[0].y;
							
							if (path_end.x == this_path_start.x &&
								path_end.y == this_path_start.y &&
								NOT_SHARED_LINKS(paths[i], paths[j]))
							{
								paths[i].next_neighbors_forward.push_back(&paths[j]);
							}
							
							if (path_start.x == this_path_start.x &&
								path_start.y == this_path_start.y &&
								NOT_SHARED_LINKS(paths[i], paths[j]))
							{
								paths[i].last_neighbors_forward.push_back(&paths[j]);
							}
							
							Vertex this_path_end;
							this_path_end.x = paths[j].verts.back().x;
							this_path_end.y = paths[j].verts.back().y;
							
							if (path_end.x == this_path_end.x &&
								path_end.y == this_path_end.y &&
								NOT_SHARED_LINKS(paths[i], paths[j]))
							{
								paths[i].next_neighbors_backward.push_back(&paths[j]);
							}
							
							if (path_start.x == this_path_start.x &&
								path_start.y == this_path_end.y &&
								NOT_SHARED_LINKS(paths[i], paths[j]))
							{
								paths[i].last_neighbors_backward.push_back(&paths[j]);
							}
						}
					}
				}
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					fprintf(stderr, "path: %p\n", &paths[i]);
					
					if (paths[i].self_closed)
					{
						fprintf(stderr, "processing self closed\n");
						
						shapes.push_back(Shape());
						shapes.back().closed = true;
						shapes.back().hole = false;
						
						for (size_t k = 0; k < paths[i].verts.size(); ++k)
						{
							shapes.back().verts.push_back(paths[i].verts[k]);
						}
						
						processShape(shapes.back(), paths[i]);
					}
				}
				
				std::vector<std::vector<Path>> closed_paths;
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					if (paths[i].fill_styles[0] != 0 || paths[i].fill_styles[1] != 0)
					{
						fprintf(stderr, "path %zu is first\n", i);
						traverse(&paths[i], closed_paths);
						break;
					}
				}
				
				for (size_t i = 0; i < closed_paths.size(); ++i)
				{
					fprintf(stderr, "got path:\n");
					
					for (size_t j = 0; j < closed_paths[i].size(); ++j)
					{
						size_t offset = (closed_paths[i][j].backward) ? -1 : 1;
						
						for (size_t k = (closed_paths[i][j].backward) ? closed_paths[i][j].verts.size() - 2 : 1; k < closed_paths[i][j].verts.size(); k += offset)
						{
							fprintf(stderr, "(%d, %d)\n", closed_paths[i][j].verts[k].x / 20, (FRAME_HEIGHT - closed_paths[i][j].verts[k].y) / 20);
						}
					}
				}
				
				fprintf(stderr, "finished traversing and processing\n");
				
				std::string tris_str = "";
				
				size_t tris_size = 0;
				
				std::vector<Shape> holes;
				
				for (size_t i = 0; i < shapes.size(); ++i)
				{
					if (shapes[i].outer_fill != 0)
					{
						fprintf(stderr, "found hole\n");
						
						shapes[i].hole = true;
						
						bool hole_found_final = false;
						
						for (int j = 0; j < shapes.size(); ++j)
						{
							if (i == j)
							{
								continue;
							}
							
							bool hole_found = true;
							
							for (int k = 0; k < shapes[i].verts.size(); ++k)
							{
								if (shapes[i].verts[k].x != shapes[j].verts[k].x || shapes[i].verts[k].y != shapes[j].verts[k].y)
								{
									hole_found = false;
									break;
								}
							}
							
							if (hole_found)
							{
								shapes[j].hole = true;
								
								if (shapes[i].fill_right)
								{
									fprintf(stderr, "hole fill right\n");
									holes.push_back(shapes[j]);
								}
								
								else
								{
									fprintf(stderr, "hole fill left\n");
									holes.push_back(shapes[i]);
								}
								
								hole_found_final = true;
								
								break;
							}
						}
						
						if (!hole_found_final)
						{
							fprintf(stderr, "hole not found, pushing original\n");
							holes.push_back(shapes[i]);
						}
					}
				}
				
				fprintf(stderr, "finished preprocessing holes\n");
				
				for (size_t i = 0; i < shapes.size(); ++i)
				{
					if (shapes[i].closed && shapes[i].inner_fill != 0 && !shapes[i].hole)
					{
						std::vector<Tri> tris;
						
						fillShape(shapes[i].verts, tris, shapes[i].fill_right);
						
						tris_size += tris.size();
						
						for (Tri t : tris)
						{
							for (int j = 0; j < 3; ++j)
							{
								tris_str += std::string("\t") + "{ "
										  + to_string(t.verts[j].x) + "/" + to_string(FRAME_WIDTH/2) + ".0f - 1.0f, "
										  + to_string(t.verts[j].y) + "/" + to_string(FRAME_HEIGHT/2) + ".0f - 1.0f, "
										  + "0.0f, "
										  + to_string(fill_styles[shapes[i].inner_fill - 1].r) + ".0f/255.0f, "
										  + to_string(fill_styles[shapes[i].inner_fill - 1].g) + ".0f/255.0f, "
										  + to_string(fill_styles[shapes[i].inner_fill - 1].b) + ".0f/255.0f, "
										  + "1.0f },\n";
							}
						}
					}
				}
				
				auto compareArea = [](const Shape& a, const Shape& b)
				{
					u64 width = a.max.x - a.min.x;
					u64 height = a.max.y - a.min.y;
					
					u64 area_a = width*height;
					
					width = b.max.x - b.min.x;
					height = b.max.y - b.min.y;
					
					u64 area_b = width*height;
					
					return area_a > area_b;
				};
				
				// Sort holes by area of bounding box
				std::sort(holes.begin(), holes.end(), compareArea);
				
				for (size_t i = 0; i < holes.size(); ++i)
				{
					std::vector<Tri> tris;
					
					fillShape(holes[i].verts, tris, holes[i].fill_right);
					
					tris_size += tris.size();
					
					for (Tri t : tris)
					{
						for (int j = 0; j < 3; ++j)
						{
							tris_str += std::string("\t") + "{ "
									  + to_string(t.verts[j].x) + "/" + to_string(FRAME_WIDTH/2) + ".0f - 1.0f, "
									  + to_string(t.verts[j].y) + "/" + to_string(FRAME_HEIGHT/2) + ".0f - 1.0f, "
									  + "0.0f, "
									  + to_string(fill_styles[holes[i].inner_fill - 1].r) + ".0f/255.0f, "
									  + to_string(fill_styles[holes[i].inner_fill - 1].g) + ".0f/255.0f, "
									  + to_string(fill_styles[holes[i].inner_fill - 1].b) + ".0f/255.0f, "
									  + "1.0f },\n";
						}
					}
				}
				
				std::string shape_name = "shape_" + to_string(shape_id) + "_tris";
				
				out_draws << endl;
				
				out_draws << "float " << shape_name << "[" << to_string(3*tris_size) << "][7] =" << endl
						  << "{" << endl;
				
				out_draws << tris_str;
				
				out_draws << "};" << endl;
				
				out_draws_header << endl << "extern float " << shape_name << "[" << to_string(3*tris_size) << "][7];" << endl;
				
				tag_main << "\t" << "dictionary[" << to_string(shape_id) << "] = (char*) " << shape_name << ";" << endl;
				tag_main << "\t" << "dictionary_sizes[" << to_string(shape_id) << "] = sizeof(" << shape_name << ");" << endl;
				
				break;
			}
		}
	}
	
	void SWF::processShape(Shape& shape, const Path& path)
	{
		s64 signed_area = 0;
		
		Vertex last_point;
		last_point.x = shape.verts[0].x;
		last_point.y = shape.verts[0].y;
		
		shape.min.x = shape.verts[0].x;
		shape.min.y = shape.verts[0].y;
		shape.max.x = shape.verts[0].x;
		shape.max.y = shape.verts[0].y;
		
		Vertex point;
		
		for (size_t k = 1; k < shape.verts.size(); ++k)
		{
			point.x = shape.verts[k].x;
			point.y = shape.verts[k].y;
			
			signed_area += CROSS(last_point, point);
			
			last_point.x = point.x;
			last_point.y = point.y;
			
			if (shape.verts[k].x < shape.min.x)
			{
				shape.min.x = shape.verts[k].x;
			}
			
			if (shape.verts[k].y < shape.min.y)
			{
				shape.min.y = shape.verts[k].y;
			}
			
			if (shape.verts[k].x > shape.max.x)
			{
				shape.max.x = shape.verts[k].x;
			}
			
			if (shape.verts[k].y > shape.max.y)
			{
				shape.max.y = shape.verts[k].y;
			}
		}
		
		point.x = shape.verts[0].x;
		point.y = shape.verts[0].y;
		
		signed_area += CROSS(last_point, point);
		
		shape.fill_right = signed_area < 0;
		
		shape.inner_fill = path.fill_styles[signed_area < 0];
		
		shape.got_min_max = true;
	}
	
	void detectCycle(Path* path, std::vector<Path>& path_stack, std::vector<Path*>& visited, std::vector<std::vector<Path>>& closed_paths)
	{
		size_t path_index = path_stack.capacity();
		
		for (size_t i = 0; i < path_stack.size(); ++i)
		{
			if (path_stack[i].original_key == path)
			{
				path_index = i;
				break;
			}
		}
		
		if (path_index != path_stack.capacity())
		{
			std::vector<Path> cycle;
			
			for (size_t i = path_index; i < path_stack.size(); ++i)
			{
				cycle.push_back(path_stack[i]);
			}
			
			closed_paths.push_back(cycle);
		}
	}
	
	void traverseBackwardIteration(Path* path, std::vector<Path>& path_stack, std::vector<Path*>& visited, std::vector<std::vector<Path>>& closed_paths);
	
	void traverseForwardIteration(Path* path, std::vector<Path>& path_stack, std::vector<Path*>& visited, std::vector<std::vector<Path>>& closed_paths)
	{
		Path* parent = nullptr;
		
		if (path_stack.size() != 0)
		{
			parent = path_stack.back().original_key;
		}
		
		path_stack.push_back(*path);
		visited.push_back(path);
		
		path_stack.back().original_key = path;
		path_stack.back().backward = false;
		
		for (Path* neighbor : path->next_neighbors_forward)
		{
			if (neighbor != parent)
			{
				detectCycle(neighbor, path_stack, visited, closed_paths);
				
				if (std::find(visited.begin(), visited.end(), neighbor) == visited.end())
				{
					traverseForwardIteration(neighbor, path_stack, visited, closed_paths);
				}
			}
		}
		
		for (Path* neighbor : path->next_neighbors_backward)
		{
			if (neighbor != parent)
			{
				detectCycle(neighbor, path_stack, visited, closed_paths);
				
				if (std::find(visited.begin(), visited.end(), neighbor) == visited.end())
				{
					traverseBackwardIteration(neighbor, path_stack, visited, closed_paths);
				}
			}
		}
		
		path_stack.pop_back();
	}
	
	void traverseBackwardIteration(Path* path, std::vector<Path>& path_stack, std::vector<Path*>& visited, std::vector<std::vector<Path>>& closed_paths)
	{
		Path* parent = nullptr;
		
		if (path_stack.size() != 0)
		{
			parent = path_stack.back().original_key;
		}
		
		path_stack.push_back(*path);
		visited.push_back(path);
		
		path_stack.back().original_key = path;
		path_stack.back().backward = true;
		
		for (Path* neighbor : path->last_neighbors_forward)
		{
			if (neighbor != parent)
			{
				detectCycle(neighbor, path_stack, visited, closed_paths);
				
				if (std::find(visited.begin(), visited.end(), neighbor) == visited.end())
				{
					traverseForwardIteration(neighbor, path_stack, visited, closed_paths);
				}
			}
		}
		
		for (Path* neighbor : path->last_neighbors_backward)
		{
			if (neighbor != parent)
			{
				detectCycle(neighbor, path_stack, visited, closed_paths);
				
				if (std::find(visited.begin(), visited.end(), neighbor) == visited.end())
				{
					traverseBackwardIteration(neighbor, path_stack, visited, closed_paths);
				}
			}
		}
		
		path_stack.pop_back();
	}
	
	void SWF::traverse(Path* path, std::vector<std::vector<Path>>& closed_paths)
	{
		std::vector<Path> path_stack;
		std::vector<Path*> visited;
		
		traverseForwardIteration(path, path_stack, visited, closed_paths);
	}
	
	/*
	 * Some notes on this next function (fillShape):
	 * 
	 * This is the fill (triangulation) algorithm I designed a year ago.
	 * Given the information that a SWF file normally provides, it is an
	 * algorithm that executes in (mostly) O(n) time, where n is the
	 * number of vertices in the shape. It accepts a list of vertices
	 * in order of the path of the shape, and outputs a list of triangles
	 * that perfectly fill the shape, with no seams or gaps to speak of.
	 * 
	 * So the big question: what does "mostly" mean? Well, on some occasions,
	 * the vertices don't play nice: two occasions in particular.
	 * 
	 * The first is when we encounter three vertices that form a
	 * concave angle. When this happens, we cannot draw the current triangle
	 * since it lies outside our shape, and we cannot (with this strategy)
	 * know where the vertices we actually want to draw really are.
	 * What happens in the current implementation, is the middle vertex of
	 * the offending angle is added to a list of "skipped" vertices
	 * (where the vertex we start from is always the first vertex
	 * in the list), which collectively become the new shape vector which
	 * is run through the function again recursively.
	 * 
	 * The second is when we encounter a vertex that goes beyond the bounds
	 * of the edges of the anchor. A bit of help from our friend the cross
	 * product can detect this condition with violent O(1) efficiency, but
	 * unfortunately it requires that we invalidate the current anchor and
	 * iterate through the shape until we find the next concave vertex,
	 * adding every single vertex along the way to the skipped vertices
	 * to be retried on the next iteration.
	 * 
	 * This recursive approach is easy enough to implement and is perfectly
	 * acceptable for the recompiler step, but I doubt I'll be pleased with
	 * the results when this algorithm is moved to the runtime (and it
	 * inevitably *must* move to the runtime, for use by ActionScript
	 * graphics commands). This *technically* performs at O(n), but
	 * performance may take a sizable hit on these edge cases.
	 * 
	 * My current thinking is written here, otherwise I would kick myself when
	 * I predictably forget months in the future. ActionScript graphics
	 * commands must declare their edges in order. They may not change the
	 * fill style before the path is finished, nor may they move the cursor.
	 * An attempt to move the cursor in particular will cause a firm
	 * finger-wagging from Flash Player as it connects the last implicit edge
	 * of your shape, fills it, and begins a new path with the same fill.
	 * Those last two assumptions alone would allow us to implement an already
	 * quite efficient version of this function, but all three of them
	 * allow us to write a vertex-crunching powerhouse.
	 * 
	 * Because the commands must execute before the shape is drawn, and
	 * in order, that allows us to calculate the cross products on the fly
	 * and cache the concave ones in an array of their indices into the shape
	 * list in ascending order. Let this array be `s32 concave[]` First, the
	 * initial iteration to find the first concave vertex vanishes, you only
	 * need to look to `concave[0]`. It can also be used to vastly improve the
	 * handling of both above edge cases.
	 * 
	 * We can use the array (after the shape has been run through one time)
	 * to fill the normally skipped vertices (since we always add concave
	 * vertices to skipped) without a recursive solution. This works at
	 * least once, though perhaps there is some nuance as to how to run
	 * again if there are more concavities. I imagine we could fill a new,
	 * shorter array of concavities as we go along and repeat until there are
	 * no concavities.
	 * 
	 * This next part I'm less sure about, but I must write it down before
	 * I lose my mind. When we encounter a vertex that is outside the bounds,
	 * we inquire as to the next concave vertex after it and move the anchor
	 * there. There are a few choices as to how to handle this from here,
	 * we may either check if the farthest vertex from the anchor breaks
	 * the edge boundaries and then fill normally, or we may enter a special
	 * loop where we temporarily fill in the opposite direction from the anchor
	 * to the farthest vertex (which, all things considered, may be a fantastic
	 * optimization in general for the algorithm, and might be possible given
	 * the concave array).
	 * 
	 * I am not willing to complicate the recompiler's fill algorithm any
	 * further, since it is not critical code and does not require complexity
	 * for the sake of performance. That time will come when the algorithm
	 * is given over to the runtime. Then, and only then, will I be willing
	 * to experiment further to obliterate this reprehensible graphics format.
	 * 
	 * Also yes, I am well aware that it's terribly written.
	 * 
	 */
	void SWF::fillShape(std::vector<Vertex>& shape, std::vector<Tri>& tris, bool fill_right)
	{
		size_t i;
		
		Tri t;
		
		size_t size = shape.size();
		
		Vertex* anchor = nullptr;
		
		Vertex* prev = nullptr;
		Vertex* prevprev = nullptr;
		
		std::vector<Vertex> skipped_vertices;
		skipped_vertices.reserve(size);
		
		for (i = 0; i < size; ++i)
		{
			Vertex* v = &shape[i];
			
			if (prevprev == nullptr)
			{
				prevprev = v;
				continue;
			}
			
			if (prev == nullptr)
			{
				prev = v;
				continue;
			}
			
			Vertex vec_a;
			Vertex vec_b;
			
			vec_a.x = prev->x - prevprev->x;
			vec_a.y = prev->y - prevprev->y;
			
			vec_b.x = v->x - prev->x;
			vec_b.y = v->y - prev->y;
			
			if (fill_right)
			{
				if (CROSS(vec_a, vec_b) > 0)
				{
					anchor = prev;
					prev = nullptr;
					prevprev = nullptr;
					break;
				}
			}
			
			else
			{
				if (CROSS(vec_a, vec_b) < 0)
				{
					anchor = prev;
					prev = nullptr;
					prevprev = nullptr;
					break;
				}
			}
			
			prevprev = prev;
			prev = v;
		}
		
		if (i == size)
		{
			anchor = &shape[0];
			i = 1;
		}
		
		skipped_vertices.push_back(*anchor);
		
		Vertex* start_shape = &shape[0];
		Vertex* started_drawing = anchor;
		Vertex* stop = &shape[size];
		prev = nullptr;
		prevprev = anchor;
		
		while (true)
		{
			Vertex* v = &shape[i];
			
			if (v == started_drawing)
			{
				if (anchor != nullptr && prev != nullptr && anchor != prev && anchor != v)
				{
					t.verts[0] = *anchor;
					t.verts[1] = *prev;
					t.verts[2] = *v;
					
					tris.push_back(t);
				}
				
				break;
			}
			
			if (anchor == nullptr)
			{
				anchor = v;
				prevprev = v;
				i += 1;
				i %= size;
				continue;
			}
			
			if (prev == nullptr)
			{
				prev = v;
				i += 1;
				i %= size;
				continue;
			}
			
			Vertex vec_prevprev_edge;
			Vertex vec_prev_edge;
			
			vec_prevprev_edge.x = prev->x - prevprev->x;
			vec_prevprev_edge.y = prev->y - prevprev->y;
			
			vec_prev_edge.x = v->x - prev->x;
			vec_prev_edge.y = v->y - prev->y;
			
			s64 cross = (fill_right) ? CROSS(vec_prevprev_edge, vec_prev_edge) : -CROSS(vec_prevprev_edge, vec_prev_edge);
			
			if (cross < 0)
			{
				size_t after_anchor_i = ((anchor - &shape[0]) + 1) % size;
				Vertex* after_anchor = &shape[after_anchor_i];
				
				if (prev != after_anchor)
				{
					size_t before_anchor_i = ((anchor - &shape[0]) - 1 + size) % size;
					Vertex* before_anchor = &shape[before_anchor_i];
					
					Vertex vec_anchor_edge;
					Vertex vec_before_anchor_edge;
					Vertex vec_anchor_new_edge;
					
					vec_anchor_edge.x = after_anchor->x - anchor->x;
					vec_anchor_edge.y = after_anchor->y - anchor->y;
					
					vec_before_anchor_edge.x = anchor->x - before_anchor->x;
					vec_before_anchor_edge.y = anchor->y - before_anchor->y;
					
					vec_anchor_new_edge.x = v->x - anchor->x;
					vec_anchor_new_edge.y = v->y - anchor->y;
					
					long first_cross = fill_right ? CROSS(vec_anchor_edge, vec_anchor_new_edge) : -CROSS(vec_anchor_edge, vec_anchor_new_edge);
					long second_cross = fill_right ? CROSS(vec_before_anchor_edge, vec_anchor_new_edge) : -CROSS(vec_before_anchor_edge, vec_anchor_new_edge);
					
					if (first_cross > 0 && second_cross > 0)
					{
						if (prev != started_drawing)
						{
							skipped_vertices.push_back(*prev);
						}
						
						if (v != started_drawing)
						{
							skipped_vertices.push_back(*v);
						}
						
						bool full_break = false;
						
						anchor = nullptr;
						
						while (anchor == nullptr)
						{
							v = &shape[i];
							
							if (v == started_drawing)
							{
								full_break = true;
								break;
							}
							
							skipped_vertices.push_back(*v);
							
							if (prevprev == nullptr)
							{
								prevprev = v;
								continue;
							}
							
							if (prev == nullptr)
							{
								prev = v;
								continue;
							}
							
							Vertex vec_a;
							Vertex vec_b;
							
							vec_a.x = prev->x - prevprev->x;
							vec_a.y = prev->y - prevprev->y;
							
							vec_b.x = v->x - prev->x;
							vec_b.y = v->y - prev->y;
							
							if (fill_right)
							{
								if (CROSS(vec_a, vec_b) > 0)
								{
									anchor = prev;
									prev = nullptr;
									prevprev = nullptr;
									break;
								}
							}
							
							else
							{
								if (CROSS(vec_a, vec_b) < 0)
								{
									anchor = prev;
									prev = nullptr;
									prevprev = nullptr;
									break;
								}
							}
							
							prevprev = prev;
							prev = v;
							
							i += 1;
							i %= size;
						}
						
						if (full_break)
						{
							break;
						}
						
						i += 1;
						i %= size;
						continue;
					}
					
					else
					{
						t.verts[0] = *anchor;
						t.verts[1] = *prev;
						t.verts[2] = *v;
						
						tris.push_back(t);
					}
				}
				
				else
				{
					t.verts[0] = *anchor;
					t.verts[1] = *prev;
					t.verts[2] = *v;
					
					tris.push_back(t);
				}
			}
			
			else if (cross == 0)
			{
				i += 1;
				i %= size;
				continue;
			}
			
			else
			{
				if (prev != started_drawing)
				{
					skipped_vertices.push_back(*prev);
				}
				
				anchor = prev;
			}
			
			prevprev = prev;
			prev = v;
			
			i += 1;
			i %= size;
		}
		
		if (skipped_vertices.size() > 1)
		{
			fillShape(skipped_vertices, tris, fill_right);
		}
	}
};