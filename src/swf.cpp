#define _USE_MATH_DEFINES
#include <algorithm>
#include <array>
#include <cmath>

#include <zlib.h>
#include <lzma.h>
#include <earcut.hpp>

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

using Coord = s32;

using N = size_t;

using Point = std::array<Coord, 2>;

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
			case SWF_TAG_DEFINE_SHAPE_2:
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
			case SWF_TAG_DEFINE_SHAPE_2:
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
				
				std::vector<FillStyle*> all_fill_styles;
				
				FillStyle* fill_styles = new FillStyle[fill_style_count];
				
				all_fill_styles.push_back(fill_styles);
				
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
				
				std::vector<LineStyle*> all_line_styles;
				
				LineStyle* line_styles = new LineStyle[line_style_count];
				
				all_line_styles.push_back(line_styles);
				
				for (u16 i = 0; i < line_style_count; ++i)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(4);
					
					shape_tag.configureNextField(SWF_FIELD_UI16, 16);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
					
					line_styles[i].width = (u16) shape_tag.fields[0].value;
					
					line_styles[i].r = (u8) shape_tag.fields[1].value;
					line_styles[i].g = (u8) shape_tag.fields[2].value;
					line_styles[i].b = (u8) shape_tag.fields[3].value;
				}
				
				shape_tag.clearFields();
				shape_tag.setFieldCount(2);
				
				shape_tag.configureNextField(SWF_FIELD_UB, 4);
				shape_tag.configureNextField(SWF_FIELD_UB, 4);
				
				shape_tag.parseFields(cur_pos);
				
				u8 fill_bits = (u8) shape_tag.fields[0].value;
				u8 line_bits = (u8) shape_tag.fields[1].value;
				
				u32 current_fill_style_list = 0;
				u32 current_line_style_list = 0;
				
				u32 last_fill_style_0 = 0;
				u32 last_fill_style_1 = 0;
				
				u32 last_line_style = 0;
				
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
						
						Vertex current;
						current.x = last_x;
						current.y = last_y;
						
						Vertex control;
						control.x = last_x + control_delta_x;
						control.y = last_y - control_delta_y;
						
						Vertex anchor;
						anchor.x = control.x + anchor_delta_x;
						anchor.y = control.y - anchor_delta_y;
						
						u32 num_passes = 6;
						
						addCurvedEdge(current_path, current, control, anchor, num_passes);
						
						last_x = anchor.x;
						last_y = anchor.y;
						
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
					
					u32 line_style = last_line_style;
					
					bool fill_style_0_change = false;
					bool fill_style_1_change = false;
					
					bool line_style_change = false;
					
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
						
						line_style_change = line_style != last_line_style;
					}
					
					if (state_new_styles)
					{
						if (cur_byte_bits_left != 8)
						{
							cur_pos += 1;
							cur_byte_bits_left = 8;
						}
						
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
						
						all_fill_styles.push_back(fill_styles);
						
						current_fill_style_list += 1;
						
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
						
						LineStyle* line_styles = new LineStyle[line_style_count];
						
						all_line_styles.push_back(line_styles);
						
						current_line_style_list += 1;
						
						for (u16 i = 0; i < line_style_count; ++i)
						{
							shape_tag.clearFields();
							shape_tag.setFieldCount(4);
							
							shape_tag.configureNextField(SWF_FIELD_UI16, 16);
							shape_tag.configureNextField(SWF_FIELD_UI8, 8);
							shape_tag.configureNextField(SWF_FIELD_UI8, 8);
							shape_tag.configureNextField(SWF_FIELD_UI8, 8);
							
							shape_tag.parseFields(cur_pos);
							
							line_styles[i].width = (u16) shape_tag.fields[0].value;
							
							line_styles[i].r = (u8) shape_tag.fields[1].value;
							line_styles[i].g = (u8) shape_tag.fields[2].value;
							line_styles[i].b = (u8) shape_tag.fields[3].value;
						}
						
						shape_tag.clearFields();
						shape_tag.setFieldCount(2);
						
						shape_tag.configureNextField(SWF_FIELD_UB, 4);
						shape_tag.configureNextField(SWF_FIELD_UB, 4);
						
						shape_tag.parseFields(cur_pos);
						
						fill_bits = (u8) shape_tag.fields[0].value;
						line_bits = (u8) shape_tag.fields[1].value;
					}
					
					if (state_new_styles || state_move_to || fill_style_0_change || fill_style_1_change || line_style_change)
					{
						if (paths.back().verts.size() == 1)
						{
							paths.pop_back();
						}
						
						paths.push_back(Path());
						current_path = &paths.back();
						
						current_path->verts.reserve(512);
						current_path->fill_style_list = current_fill_style_list;
						current_path->line_style_list = current_line_style_list;
						current_path->fill_styles[0] = fill_style_0;
						current_path->fill_styles[1] = fill_style_1;
						current_path->line_style = line_style;
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
				
				std::vector<Shape> shapes;
				
				std::vector<Node> nodes;
				
				constructEdges(paths, nodes);
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					if (paths[i].self_closed)
					{
						shapes.push_back(Shape());
						shapes.back().closed = true;
						shapes.back().hole = false;
						shapes.back().invalid = false;
						
						for (size_t k = 0; k < paths[i].verts.size(); ++k)
						{
							if (k >= 1 &&
								shapes.back().verts.back().x == paths[i].verts[k].x &&
								shapes.back().verts.back().y == paths[i].verts[k].y)
							{
								continue;
							}
							
							shapes.back().verts.push_back(paths[i].verts[k]);
						}
						
						processShape(shapes.back(), paths[i].fill_styles);
						
						shapes.back().fill_style_list = paths[i].fill_style_list;
						
						if (paths[i].fill_styles[shapes.back().fill_right] == 0 && paths[i].fill_styles[!shapes.back().fill_right] != 0)
						{
							shapes.back().hole = true;
							shapes.back().outer_fill = paths[i].fill_styles[!shapes.back().fill_right];
						}
					}
				}
				
				std::vector<Path> path_stack;
				std::unordered_map<Node*, bool> blocked;
				std::unordered_map<Node*, std::vector<Node*>> blocked_map;
				std::vector<std::vector<Path>> closed_paths;
				
				johnson(nodes, path_stack, blocked, blocked_map, closed_paths);
				
				size_t shape_cycles_start = shapes.size();
				
				for (auto cycle : closed_paths)
				{
					shapes.push_back(Shape());
					shapes.back().closed = true;
					shapes.back().hole = false;
					shapes.back().invalid = false;
					
					for (size_t j = 0; j < cycle.size(); ++j)
					{
						size_t start = (cycle[j].backward) ? cycle[j].verts.size() - 2 : 1;
						size_t offset = (cycle[j].backward) ? -1 : 1;
						
						for (size_t k = start; k < cycle[j].verts.size(); k += offset)
						{
							if (shapes.back().verts.back().x == cycle[j].verts[k].x &&
								shapes.back().verts.back().y == cycle[j].verts[k].y)
							{
								continue;
							}
							
							shapes.back().verts.push_back(cycle[j].verts[k]);
						}
					}
					
					processShape(shapes.back(), cycle[0].fill_styles);
					
					shapes.back().fill_style_list = cycle[0].fill_style_list;
				}
				
				for (size_t i = 0; i < closed_paths.size(); ++i)
				{
					Shape& shape = shapes[shape_cycles_start + i];
					std::vector<Path>& cycle = closed_paths[i];
					
					u32 last_fill_style = fill_style_count + 1;
					
					for (size_t j = 0; j < cycle.size(); ++j)
					{
						Path& p = cycle[j];
						if (last_fill_style == fill_style_count + 1)
						{
							last_fill_style = p.fill_styles[shape.fill_right];
							continue;
						}
						
						if (last_fill_style != p.fill_styles[shape.fill_right ^ p.backward])
						{
							shape.invalid = true;
							break;
						}
					}
					
					// TODO: look for holes here too?
				}
				
				for (size_t i = 0; i < shapes.size(); ++i)
				{
					if (shapes[i].verts.size() < 3)
					{
						shapes[i].invalid = true;
					}
				}
				
				std::string tris_str = "";
				
				size_t tris_size = 0;
				
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
				
				auto compareAreaPtr = [](const Shape* a, const Shape* b)
				{
					u64 width = a->max.x - a->min.x;
					u64 height = a->max.y - a->min.y;
					
					u64 area_a = width*height;
					
					width = b->max.x - b->min.x;
					height = b->max.y - b->min.y;
					
					u64 area_b = width*height;
					
					return area_a > area_b;
				};
				
				// Sort shapes by area of bounding box
				std::sort(shapes.begin(), shapes.end(), compareArea);
				
				for (size_t i = 0; i < shapes.size(); ++i)
				{
					if (shapes[i].hole)
					{
						Shape& hole = shapes[i];
						
						std::vector<Shape*> outer_candidates;
						
						for (size_t j = 0; j < shapes.size(); ++j)
						{
							if (shapes[j].invalid)
							{
								continue;
							}
							
							Shape& test_shape = shapes[j];
							
							if (test_shape.min.x < hole.min.x && test_shape.max.x > hole.max.x &&
								test_shape.min.y < hole.min.y && test_shape.max.y > hole.max.y)
							{
								outer_candidates.push_back(&test_shape);
							}
						}
						
						std::vector<Shape*> final_outer_candidates;
						
						for (Shape* c : outer_candidates)
						{
							bool v_in_c = true;
							
							for (const Vertex& v : hole.verts)
							{
								if (!isInShape(v, c))
								{
									v_in_c = false;
									break;
								}
							}
							
							if (v_in_c)
							{
								final_outer_candidates.push_back(c);
							}
						}
						
						std::sort(final_outer_candidates.begin(), final_outer_candidates.end(), compareAreaPtr);
						
						final_outer_candidates.back()->holes.push_back(&hole);
					}
				}
				
				for (size_t i = 0; i < shapes.size(); ++i)
				{
					if (!shapes[i].invalid && shapes[i].closed && shapes[i].inner_fill != 0 && !shapes[i].hole)
					{
						std::vector<Tri> tris;
						
						fillShape(shapes[i], tris);
						
						tris_size += tris.size();
						
						for (Tri t : tris)
						{
							for (int j = 0; j < 3; ++j)
							{
								tris_str += std::string("\t") + "{ "
										  + to_string(t.verts[j].x) + "/" + to_string(FRAME_WIDTH/2) + ".0f - 1.0f, "
										  + to_string(t.verts[j].y) + "/" + to_string(FRAME_HEIGHT/2) + ".0f - 1.0f, "
										  + "0.0f, "
										  + to_string(all_fill_styles[shapes[i].fill_style_list][shapes[i].inner_fill - 1].r) + ".0f/255.0f, "
										  + to_string(all_fill_styles[shapes[i].fill_style_list][shapes[i].inner_fill - 1].g) + ".0f/255.0f, "
										  + to_string(all_fill_styles[shapes[i].fill_style_list][shapes[i].inner_fill - 1].b) + ".0f/255.0f, "
										  + "1.0f },\n";
							}
						}
					}
				}
				
				for (size_t i = 0; i < paths.size(); ++i)
				{
					u8 line_style_i = paths[i].line_style;
					
					if (line_style_i != 0)
					{
						LineStyle line_style = all_line_styles[paths[i].line_style_list][line_style_i - 1];
						
						std::vector<Tri> tris;
						
						drawLines(paths[i], line_style.width, tris);
						
						tris_size += tris.size();
						
						for (Tri t : tris)
						{
							for (int j = 0; j < 3; ++j)
							{
								tris_str += std::string("\t") + "{ "
										  + to_string(t.verts[j].x) + "/" + to_string(FRAME_WIDTH/2) + ".0f - 1.0f, "
										  + to_string(t.verts[j].y) + "/" + to_string(FRAME_HEIGHT/2) + ".0f - 1.0f, "
										  + "0.0f, "
										  + to_string(line_style.r) + ".0f/255.0f, "
										  + to_string(line_style.g) + ".0f/255.0f, "
										  + to_string(line_style.b) + ".0f/255.0f, "
										  + "1.0f },\n";
							}
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
	
	s32 pointOrientation(const Vertex& v0, const Vertex& v1, const Vertex& point)
	{
		return (v1.x - v0.x)*(point.y - v0.y) - (point.x - v0.x)*(v1.y - v0.y);
	}
	
	bool SWF::isInShape(const Vertex& v, const Shape* shape)
	{
		const Vertex* last_outer_v = nullptr;
		
		int windingNumber = 0;
		
		for (const Vertex& outer_v : shape->verts)
		{
			if (last_outer_v == nullptr)
			{
				last_outer_v = &outer_v;
				continue;
			}
			
			if (last_outer_v->y <= v.y)
			{
				if (outer_v.y > v.y && pointOrientation(*last_outer_v, outer_v, v) > 0)
				{
					windingNumber += 1;
				}
			}
			
			else
			{
				if (outer_v.y <= v.y && pointOrientation(*last_outer_v, outer_v, v) < 0)
				{
					windingNumber -= 1;
				}
			}
			
			last_outer_v = &outer_v;
		}
		
		return windingNumber != 0;
	}
	
	void SWF::addCurvedEdge(Path* path, Vertex current, Vertex control, Vertex anchor, u32 passes)
	{
		std::vector<Vertex> left_points;
		std::vector<Vertex> right_points;
		
		for (u32 i = 1; i <= passes; ++i)
		{
			float t = (float) i / passes;
			float u = 1.0f - t;
			
			s32 x = (s32) (u*u*current.x + 2*u*t*control.x + t*t*anchor.x);
			s32 y = (s32) (u*u*current.y + 2*u*t*control.y + t*t*anchor.y);
			
			Vertex v;
			v.x = x;
			v.y = y;
			
			path->verts.push_back(v);
		}
	}
	
	void SWF::processShape(Shape& shape, u32* fill_styles)
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
		
		shape.inner_fill = fill_styles[signed_area < 0];
		
		shape.got_min_max = true;
	}
	
	void SWF::constructEdges(std::vector<Path>& paths, std::vector<Node>& nodes)
	{
		nodes.reserve(2*paths.size());
		
		for (size_t i = 0; i < nodes.capacity(); ++i)
		{
			nodes.push_back(Node());
			nodes.back().used = false;
		}
		
		for (size_t i = 0; i < paths.size(); ++i)
		{
			Node* front = &nodes[2*i];
			Node* back = &nodes[2*i + 1];
			
			front->path = &paths[i];
			back->path = &paths[i];
			
			paths[i].front = front;
			paths[i].back = back;
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
				
				Node* front = paths[i].front;
				Node* back = paths[i].back;
				
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
						path_end.y == this_path_start.y)
					{
						back->neighbors.push_back(paths[j].back);
					}
					
					if (path_start.x == this_path_start.x &&
						path_start.y == this_path_start.y)
					{
						front->neighbors.push_back(paths[j].back);
					}
					
					Vertex this_path_end;
					this_path_end.x = paths[j].verts.back().x;
					this_path_end.y = paths[j].verts.back().y;
					
					if (path_end.x == this_path_end.x &&
						path_end.y == this_path_end.y)
					{
						back->neighbors.push_back(paths[j].front);
					}
					
					if (path_start.x == this_path_end.x &&
						path_start.y == this_path_end.y)
					{
						front->neighbors.push_back(paths[j].front);
					}
				}
			}
		}
	}
	
	void blockInMap(Node* node, std::unordered_map<Node*, std::vector<Node*>>& blocked_map)
	{
		blocked_map[node].clear();
		
		for (Node* neighbor : node->neighbors)
		{
			blocked_map[node].push_back(neighbor);
		}
	}
	
	void unblock(Node* node, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map)
	{
		blocked[node] = false;
		
		for (Node* n : blocked_map[node])
		{
			if (blocked[n])
			{
				unblock(n, blocked, blocked_map);
			}
		}
		
		blocked_map[node].clear();
	}
	
	bool traverseIteration(Node* path, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths);
	
	bool detectCycle(Node* node, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths)
	{
		if (node == path_stack[0].front || node == path_stack[0].back)
		{
			std::vector<Path> cycle;
			
			for (size_t i = 0; i < path_stack.size(); ++i)
			{
				cycle.push_back(path_stack[i]);
			}
			
			closed_paths.push_back(cycle);
			
			return true;
		}
		
		if (blocked[node])
		{
			return false;
		}
		
		return traverseIteration(node, path_stack, blocked, blocked_map, closed_paths);
	}
	
	bool traverseIteration(Node* node, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths)
	{
		path_stack.push_back(*node->path);
		
		path_stack.back().backward = node == node->path->front;
		
		blocked[node] = true;
		
		bool cycle_found = false;
		
		for (Node* neighbor : node->neighbors)
		{
			if (neighbor->used)
			{
				continue;
			}
			
			cycle_found |= detectCycle(neighbor, path_stack, blocked, blocked_map, closed_paths);
		}
		
		path_stack.pop_back();
		
		if (cycle_found)
		{
			unblock(node, blocked, blocked_map);
			return true;
		}
		
		blockInMap(node, blocked_map);
		
		return false;
	}
	
	void SWF::johnson(std::vector<Node>& nodes, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths)
	{
		for (Node& n : nodes)
		{
			blocked.clear();
			blocked_map.clear();
			
			traverseIteration(&n, path_stack, blocked, blocked_map, closed_paths);
			n.used = true;
		}
	}
	
	void SWF::fillShape(Shape& shape, std::vector<Tri>& tris)
	{
		std::vector<std::vector<std::array<Coord, 2>>> polygon;
		std::vector<std::array<Coord, 2>> shape_array;
		std::array<Coord, 2> array;
		
		std::vector<std::array<Coord, 2>> all_points;
		
		for (const Vertex& v : shape.verts)
		{
			array[0] = v.x;
			array[1] = v.y;
			
			shape_array.push_back(array);
			
			all_points.push_back(array);
		}
		
		polygon.push_back(shape_array);
		
		for (const Shape* h : shape.holes)
		{
			shape_array.clear();
			
			for (const Vertex& v : h->verts)
			{
				array[0] = v.x;
				array[1] = v.y;
				
				shape_array.push_back(array);
				
				all_points.push_back(array);
			}
			
			polygon.push_back(shape_array);
		}
		
		std::vector<N> indices = mapbox::earcut<N>(polygon);
		
		Tri t;
		
		for (size_t i = 0; i < indices.size(); ++i)
		{
			size_t tri_index = i % 3;
			
			t.verts[tri_index].x = all_points[indices[i]][0];
			t.verts[tri_index].y = all_points[indices[i]][1];
			
			if (tri_index == 2)
			{
				tris.push_back(t);
			}
		}
	}
	
	void SWF::drawLines(const Path& path, u16 width, std::vector<Tri>& tris)
	{
		if (width != 0)
		{
			width = (u16) std::max(width, (u16) 20);
		}
		
		u16 halfwidth = width/2;
		
		Vertex last_v = path.verts[0];
		
		for (Vertex v : path.verts)
		{
			Tri t;
			
			double angle = atan2(v.y - last_v.y, v.x - last_v.x) - M_PI/2.0;
			
			t.verts[0].x = last_v.x + ((s32) std::round(halfwidth*cos(angle)));
			t.verts[0].y = last_v.y + ((s32) std::round(halfwidth*sin(angle)));
			
			t.verts[2].x = v.x + ((s32) std::round(halfwidth*cos(angle)));
			t.verts[2].y = v.y + ((s32) std::round(halfwidth*sin(angle)));
			
			angle += M_PI;
			
			t.verts[1].x = last_v.x + ((s32) std::round(halfwidth*cos(angle)));
			t.verts[1].y = last_v.y + ((s32) std::round(halfwidth*sin(angle)));
			
			tris.push_back(t);
			
			t.verts[0].x = v.x + ((s32) std::round(halfwidth*cos(angle)));
			t.verts[0].y = v.y + ((s32) std::round(halfwidth*sin(angle)));
			
			tris.push_back(t);
			
			last_v = v;
		}
	}
};