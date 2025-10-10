#define _USE_MATH_DEFINES
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>

#include <zlib.h>
#include <lzma.h>
#include <earcut.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
	
	SWF::SWF(Context& context) : num_finished_tags(0),
								 next_frame_i(0),
								 another_frame(false),
								 next_script_i(0),
								 last_queued_script(0),
								 current_tri(0),
								 current_transform(0),
								 current_color(0),
								 current_uninv(0),
								 current_gradient(0),
								 current_bitmap_pixel(0),
								 current_bitmap(0),
								 jpeg_tables(nullptr)
	{
		// Configure reusable struct records
		// 
		// Using a SWFTag without parsing the header
		// behaves exactly like a SWF struct record
		RGB.setFieldCount(3);
		RGB.configureNextField(SWF_FIELD_UI8);  // Red
		RGB.configureNextField(SWF_FIELD_UI8);  // Green
		RGB.configureNextField(SWF_FIELD_UI8);  // Blue
		
		printf("Reading %s...\n", context.swf_path.c_str());
		
		ifstream swf_file(context.swf_path, ios_base::in | ios_base::binary);
		if (!swf_file.good())
		{
			EXC_ARG("SWF file `%s' not found\n", context.swf_path.c_str());
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
		
		std::string width = to_string(FRAME_WIDTH/20);
		std::string height = to_string(FRAME_HEIGHT/20);
		std::string width_twips = to_string(FRAME_WIDTH);
		std::string height_twips = to_string(FRAME_HEIGHT);
		
		context.constants_header << "#define FRAME_WIDTH " << width << endl
								 << "#define FRAME_HEIGHT " << height << endl
								 << "#define FRAME_WIDTH_TWIPS " << width_twips << endl
								 << "#define FRAME_HEIGHT_TWIPS " << height_twips << endl << endl
								 << "extern const float stage_to_ndc[16];";
		
		context.constants << "#include \"constants.h\"" << endl << endl
						  << "const float stage_to_ndc[16] =" << endl
						  << "{" << endl
						  << "\t" << "1.0f/(FRAME_WIDTH_TWIPS/2.0f)," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "-1.0f/(FRAME_HEIGHT_TWIPS/2.0f)," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "1.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "-1.0f," << endl
						  << "\t" << "1.0f," << endl
						  << "\t" << "0.0f," << endl
						  << "\t" << "1.0f," << endl
						  << "};";
	}
	
	void SWF::parseMatrix(MATRIX& matrix_out)
	{
		u32 cur_byte_bits_left = 8;
		
		SWFTag matrix_tag;
		
		matrix_tag.clearFields();
		matrix_tag.setFieldCount(1);
		
		matrix_tag.configureNextField(SWF_FIELD_UB, 1);
		
		matrix_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
		
		bool has_scale = matrix_tag.fields[0].value & 1;
		
		matrix_out.scale_x = 1;
		matrix_out.scale_y = 1;
		
		if (has_scale)
		{
			matrix_tag.clearFields();
			matrix_tag.setFieldCount(3);
			
			matrix_tag.configureNextField(SWF_FIELD_UB, 5, true);
			matrix_tag.configureNextField(SWF_FIELD_FB, 0);
			matrix_tag.configureNextField(SWF_FIELD_FB, 0);
			
			matrix_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
			
			matrix_out.scale_x = VAL(float, &matrix_tag.fields[1].value);
			matrix_out.scale_y = VAL(float, &matrix_tag.fields[2].value);
		}
		
		matrix_tag.clearFields();
		matrix_tag.setFieldCount(1);
		
		matrix_tag.configureNextField(SWF_FIELD_UB, 1);
		
		matrix_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
		
		bool has_rotate = matrix_tag.fields[0].value & 1;
		
		matrix_out.rotateskew_0 = 0;
		matrix_out.rotateskew_1 = 0;
		
		if (has_rotate)
		{
			matrix_tag.clearFields();
			matrix_tag.setFieldCount(3);
			
			matrix_tag.configureNextField(SWF_FIELD_UB, 5, true);
			matrix_tag.configureNextField(SWF_FIELD_FB, 0);
			matrix_tag.configureNextField(SWF_FIELD_FB, 0);
			
			matrix_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
			
			matrix_out.rotateskew_0 = VAL(float, &matrix_tag.fields[1].value);
			matrix_out.rotateskew_1 = VAL(float, &matrix_tag.fields[2].value);
		}
		
		matrix_tag.clearFields();
		matrix_tag.setFieldCount(3);
		
		matrix_tag.configureNextField(SWF_FIELD_UB, 5, true);
		matrix_tag.configureNextField(SWF_FIELD_SB, 0);
		matrix_tag.configureNextField(SWF_FIELD_SB, 0);
		
		matrix_tag.parseFieldsContinue(cur_pos, cur_byte_bits_left);
		
		matrix_out.translate_x = (s32) matrix_tag.fields[1].value;
		matrix_out.translate_y = (s32) matrix_tag.fields[2].value;
		
		if (cur_byte_bits_left != 8)
		{
			cur_pos += 1;
		}
	}
	
	void SWF::parseAllTags(Context& context)
	{
		SWFTag tag;
		
		context.tag_main << "#include <recomp.h>" << endl << endl
				 << "#include <out.h>" << endl
				 << "#include \"draws.h\"" << endl << endl
				 << "void frame_" << to_string(next_frame_i) << "()" << endl
				 << "{" << endl;
		next_frame_i += 1;
		
		context.out_script_header = ofstream(context.output_scripts_folder + "out.h", ios_base::out);
		context.out_script_header << "#pragma once" << endl;
		
		context.out_script_defs = ofstream(context.output_scripts_folder + "script_defs.c", ios_base::out);
		context.out_script_defs << "#include \"script_decls.h\"" << endl;
		
		context.out_script_decls = ofstream(context.output_scripts_folder + "script_decls.h", ios_base::out);
		context.out_script_decls << "#pragma once" << endl << endl
								 << "#include <stackvalue.h>" << endl;
		
		// output identity matrix at transform id 0
		transform_data << "\t" << "1.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   
					   << "\t" << "0.0f," << endl
					   << "\t" << "1.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "1.0f," << endl
					   << "\t" << "0.0f," << endl
					   
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "0.0f," << endl
					   << "\t" << "1.0f," << endl;
		
		current_transform += 1;
		
		// prime the loop
		tag.code = (TagType) 1;
		
		while (tag.code != 0)
		{
			tag.parseHeader(cur_pos);
			interpretTag(context, tag);
			tag.clearFields();
		}
		
		context.tag_main << endl << endl
						 << "typedef void (*frame_func)();" << endl << endl
						 << "frame_func frame_funcs[] =" << endl
						 << "{" << endl;
		
		for (size_t i = 0; i < next_frame_i; ++i)
		{
			context.tag_main << "\t" << "frame_" << to_string(i) << "," << endl;
		}
		
		if (current_bitmap_pixel)
		{
			tag_init << endl << "\tfinalizeBitmaps();";
		}
		
		context.tag_main << "};" << endl
						 << endl
						 << "void tagInit()" << endl
						 << "{"
						 << tag_init.str() << endl
						 << "}";
		
		context.out_draws << endl << endl;
		
		context.out_draws << "u32 shape_data[" << to_string(current_tri ? 3*current_tri : 1) << "][4] =" << endl
						  << "{" << endl
						  << (current_tri ? shape_data.str() : "\t0\n")
						  << "};" << endl
						  << endl
						  << "float transform_data[" << to_string(current_transform ? current_transform : 1) << "][16] =" << endl
						  << "{" << endl
						  << (current_transform ? transform_data.str() : "\t0\n")
						  << "};" << endl
						  << endl
						  << "float color_data[" << to_string(current_color ? current_color : 1) << "][4] =" << endl
						  << "{" << endl
						  << (current_color ? color_data.str() : "\t0\n")
						  << "};" << endl
						  << endl
						  << "float uninv_mat_data[" << to_string(current_uninv ? 16*current_uninv : 1) << "] =" << endl
						  << "{" << endl
						  << (current_uninv ? uninv_mat_data.str() : "\t0\n")
						  << "};" << endl
						  << endl
						  << "u8 gradient_data[" << to_string(current_gradient ? 256*current_gradient : 1) << "][4] =" << endl
						  << "{" << endl
						  << (current_gradient ? gradient_data.str() : "\t0\n")
						  << "};" << endl
						  << endl
						  << "u8 bitmap_data[" << to_string(current_bitmap_pixel ? 4*current_bitmap_pixel : 1) << "] =" << endl
						  << "{" << endl
						  << (current_bitmap_pixel ? bitmap_data.str() : "\t0\n")
						  << "};";
		
		context.out_draws_header << endl
								 << "extern u32 shape_data[" << to_string(current_tri ? 3*current_tri : 1) << "][4];" << endl
								 << "extern float transform_data[" << to_string(current_transform ? current_transform : 1) << "][16];" << endl
								 << "extern float color_data[" << to_string(current_color ? current_color : 1) << "][4];" << endl
								 << "extern float uninv_mat_data[" << to_string(current_uninv ? 16*current_uninv : 1) << "];" << endl
								 << "extern u8 gradient_data[" << to_string(current_gradient ? 256*current_gradient : 1) << "][4];" << endl
								 << "extern u8 bitmap_data[" << to_string(current_bitmap_pixel ? 4*current_bitmap_pixel : 1) << "];";
		
		size_t highest_w = 0;
		size_t highest_h = 0;
		
		for (const Vertex& v : bitmap_sizes)
		{
			if (v.x > highest_w)
			{
				highest_w = v.x;
			}
			
			if (v.y > highest_h)
			{
				highest_h = v.y;
			}
		}
		
		context.constants_header << endl << endl
								 << "#define BITMAP_COUNT " << to_string(current_bitmap) << endl
								 << "#define BITMAP_HIGHEST_W " << to_string(highest_w) << endl
								 << "#define BITMAP_HIGHEST_H " << to_string(highest_h);
		
		context.out_script_header.close();
		context.out_script_defs.close();
		context.out_script_decls.close();
	}
	
	void SWF::interpretTag(Context& context, SWFTag& tag)
	{
		printf("tag code: %d, tag length: %d\n", tag.code, tag.length);
		
		if (another_frame && tag.code != SWF_TAG_END_TAG)
		{
			context.tag_main << "}" << endl << endl
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
					context.tag_main << "\t" << "quit_swf = 1;" << endl;
				}
				
				else
				{
					context.tag_main << "\t" << "if (!manual_next_frame)" << endl
									 << "\t" << "{" << endl
									 << "\t\t" << "next_frame = 0;" << endl
									 << "\t\t" << "manual_next_frame = 1;" << endl
									 << "\t" << "}" << endl;
				}
				
				context.tag_main << "}";
				
				break;
			}
			
			case SWF_TAG_SHOW_FRAME:
			{
				while (last_queued_script < next_script_i)
				{
					context.tag_main << "\t" << "script_" << to_string(last_queued_script) << "(stack, &sp);" << endl;
					last_queued_script += 1;
				}
				
				context.tag_main << "\t" << "tagShowFrame();" << endl;
				
				another_frame = true;
				
				break;
			}
			
			case SWF_TAG_DEFINE_BITS:
			{
				if (jpeg_tables == nullptr)
				{
					EXC("JPEG bitmap tag encountered before JPEGTables!\n");
				}
				
				size_t new_length = tag.length;
				
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				u16 char_id = (u16) tag.fields[0].value;
				new_length -= 2;
				
				// stupid swf edge cases are stupid
				if ((u8) cur_pos[0] == 0xFF &&
					(u8) cur_pos[1] == 0xD9 &&
					(u8) cur_pos[2] == 0xFF &&
					(u8) cur_pos[3] == 0xD8)
				{
					cur_pos += 4;
					new_length -= 4;
				}
				
				else if ((u8) cur_pos[0] == 0xFF &&
						 (u8) cur_pos[1] == 0xD8)
				{
					cur_pos += 2;
					new_length -= 2;
				}
				
				size_t jpeg_data_size = new_length + jpeg_tables_size;
				u8* jpeg_data = new u8[jpeg_data_size];
				
				for (size_t i = 0; i < jpeg_tables_size; ++i)
				{
					jpeg_data[i] = jpeg_tables[i];
				}
				
				for (size_t i = jpeg_tables_size; i < jpeg_data_size; ++i)
				{
					jpeg_data[i] = cur_pos[i - jpeg_tables_size];
				}
				
				int w;
				int h;
				int comp;
				u8* decompressed = stbi_load_from_memory(jpeg_data, (int) jpeg_data_size, &w, &h, &comp, 3);
				
				if (decompressed == nullptr)
				{
					EXC("JPEG data returned NULL.\n");
				}
				
				Vertex v;
				v.x = w;
				v.y = h;
				
				bitmap_sizes.push_back(v);
				
				size_t bitmap_start = current_bitmap_pixel;
				
				for (size_t i = 0; i < 3*w*h; i += 3)
				{
					bitmap_data << std::hex << std::uppercase << std::setw(2)
								<< "\t0x" << (u32) decompressed[i] << "," << endl
								<< "\t0x" << (u32) decompressed[i + 1] << "," << endl
								<< "\t0x" << (u32) decompressed[i + 2] << "," << endl
								<< "\t0xFF," << endl;
					
					current_bitmap_pixel += 1;
				}
				
				char_id_to_bitmap_id[char_id] = current_bitmap;
				
				tag_init << endl
						 << "\tdefineBitmap("
						 << to_string(4*bitmap_start) << ", "
						 << to_string(4*(current_bitmap_pixel - bitmap_start)) << ", "
						 << to_string(w) << ", "
						 << to_string(h)
						 << ");";
				
				current_bitmap += 1;
				
				cur_pos += new_length;
				
				break;
			}
			
			case SWF_TAG_JPEG_TABLES:
			{
				if (jpeg_tables != nullptr)
				{
					EXC("More than one JPEGTables tag detected.\n");
				}
				
				size_t new_length = tag.length;
				
				if ((u8) cur_pos[0] == 0xFF &&
					(u8) cur_pos[1] == 0xD9 &&
					(u8) cur_pos[2] == 0xFF &&
					(u8) cur_pos[3] == 0xD8)
				{
					cur_pos += 2;
					new_length -= 2;
				}
				
				jpeg_tables = new u8[new_length - 2];
				jpeg_tables_size = new_length - 2;
				
				for (size_t i = 0; i < new_length - 2; ++i)
				{
					jpeg_tables[i] = cur_pos[i];
				}
				
				cur_pos += new_length;
				
				break;
			}
			
			case SWF_TAG_DEFINE_SHAPE:
			case SWF_TAG_DEFINE_SHAPE_2:
			{
				interpretShape(context, tag);
				
				break;
			}
			
			case SWF_TAG_SET_BACKGROUND_COLOR:
			{
				RGB.parseFields(cur_pos);
				
				context.tag_main << "\t" << "tagSetBackgroundColor("
								 << to_string((u8) RGB.fields[0].value) << ", "
								 << to_string((u8) RGB.fields[1].value) << ", "
								 << to_string((u8) RGB.fields[2].value) << ");" << endl;
				
				break;
			}
			
			case SWF_TAG_DEFINE_FONT:
			{
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				u16 font_id = (u16) tag.fields[0].value;
				
				char* offset_table = cur_pos;
				
				tag.clearFields();
				tag.setFieldCount(1);
				
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				std::vector<u16> entry_offsets;
				entry_offsets.push_back((u16) tag.fields[0].value);
				
				u16 num_entries = entry_offsets.back()/2;
				
				tag.clearFields();
				tag.setFieldCount(num_entries - 1);
				
				for (u16 i = 0; i < num_entries - 1; ++i)
				{
					tag.configureNextField(SWF_FIELD_UI16);
				}
				
				tag.parseFields(cur_pos);
				
				for (u16 i = 0; i < num_entries - 1; ++i)
				{
					entry_offsets.push_back((u16) tag.fields[i].value);
				}
				
				for (u16 i = 0; i < num_entries; ++i)
				{
					cur_pos = offset_table + entry_offsets[i];
					interpretShape(context, tag);
				}
				
				break;
			}
			
			case SWF_TAG_DO_ACTION:
			{
				context.out_script_header << endl << "void script_" << to_string(next_script_i) << "(char* stack, u32* sp);";
				
				ofstream out_script(context.output_scripts_folder + "script_" + to_string(next_script_i) + ".c", ios_base::out);
				out_script << "#include <recomp.h>" << endl
						   << "#include \"script_decls.h\"" << endl << endl
						   << "void script_" << next_script_i << "(char* stack, u32* sp)" << endl
						   << "{" << endl;
				
				next_script_i += 1;
				
				action.parseActions(context, cur_pos, out_script);
				
				out_script << "}";
				
				break;
			}
			
			case SWF_TAG_DEFINE_FONT_INFO:
			{
				cur_pos += tag.length;
				
				break;
			}
			
			case SWF_TAG_PLACE_OBJECT_2:
			{
				tag.setFieldCount(2);
				
				tag.configureNextField(SWF_FIELD_UI8);
				tag.configureNextField(SWF_FIELD_UI16);
				
				tag.parseFields(cur_pos);
				
				u8 flags = (u8) tag.fields[0].value;
				u16 depth = (u16) tag.fields[1].value;
				
				// TODO: SWF 5 and up uses PlaceFlagHasClipActions
				
				bool has_clip_actions = (flags & 0b10000000) != 0;
				bool has_clip_depth = (flags & 0b01000000) != 0;
				bool has_name = (flags & 0b00100000) != 0;
				bool has_ratio = (flags & 0b00010000) != 0;
				bool has_color = (flags & 0b00001000) != 0;
				bool has_matrix = (flags & 0b00000100) != 0;
				bool has_character = (flags & 0b00000010) != 0;
				bool move = (flags & 0b00000001) != 0;
				
				u16 char_id = 0;
				
				if (has_character)
				{
					tag.clearFields();
					tag.setFieldCount(1);
					
					tag.configureNextField(SWF_FIELD_UI16);
					
					tag.parseFields(cur_pos);
					
					char_id = (u16) tag.fields[0].value;
				}
				
				std::string transform_name = "transform_" + to_string(num_finished_tags);
				
				size_t transform_id = current_transform;
				
				if (has_matrix)
				{
					MATRIX matrix;
					parseMatrix(matrix);
					
					recompileMatrix(matrix, transform_data);
					current_transform += 1;
				}
				
				else
				{
					transform_id = 0;
				}
				
				context.tag_main << "\t" << "tagPlaceObject2(" << to_string(depth) << ", " << to_string(char_id) << ", " << to_string(transform_id) << ");" << endl;
				
				current_transform += 1;
				
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
				
				context.tag_main << "\t" << "tagScriptLimits("
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
		
		num_finished_tags += 1;
	}
	
	u8 rgbLerp(u8 start, u8 end, float t)
	{
		int diff = end - start;
		return (u8) (start + t*diff);
	}
	
	void SWF::recompileMatrix(MATRIX matrix, std::stringstream& out)
	{
		out << std::fixed << std::setprecision(15)
			<< "\t" << matrix.scale_x << "f," << endl
			<< "\t" << matrix.rotateskew_0 << "f," << endl
			<< "\t" << "0.0f," << endl
			<< "\t" << "0.0f," << endl
			
			<< "\t" << matrix.rotateskew_1 << "f," << endl
			<< "\t" << matrix.scale_y << "f," << endl
			<< "\t" << "0.0f," << endl
			<< "\t" << "0.0f," << endl
			
			<< "\t" << "0.0f," << endl
			<< "\t" << "0.0f," << endl
			<< "\t" << "1.0f," << endl
			<< "\t" << "0.0f," << endl
			
			<< "\t" << (float) matrix.translate_x << "f," << endl
			<< "\t" << (float) matrix.translate_y << "f," << endl
			<< "\t" << "0.0f," << endl
			<< "\t" << "1.0f," << endl;
	}
	
	FillStyle* SWF::parseFillStyles(u16 fill_style_count)
	{
		SWFTag fill_data;
		
		FillStyle* fill_styles = new FillStyle[fill_style_count];
		
		for (u16 i = 0; i < fill_style_count; ++i)
		{
			fill_data.clearFields();
			fill_data.setFieldCount(1);
			
			fill_data.configureNextField(SWF_FIELD_UI8, 8);
			
			fill_data.parseFields(cur_pos);
			
			fill_styles[i].type = (u8) fill_data.fields[0].value;
			
			switch (fill_styles[i].type)
			{
				case FILL_SOLID:
				{
					fill_data.clearFields();
					fill_data.setFieldCount(3);
					
					fill_data.configureNextField(SWF_FIELD_UI8, 8);
					fill_data.configureNextField(SWF_FIELD_UI8, 8);
					fill_data.configureNextField(SWF_FIELD_UI8, 8);
					
					fill_data.parseFields(cur_pos);
					
					fill_styles[i].r = (u8) fill_data.fields[0].value;
					fill_styles[i].g = (u8) fill_data.fields[1].value;
					fill_styles[i].b = (u8) fill_data.fields[2].value;
					
					fill_styles[i].index = current_color;
					
					color_data << "\t" << "{ "
							   << to_string(fill_styles[i].r) << "/255.0f, "
							   << to_string(fill_styles[i].g) << "/255.0f, "
							   << to_string(fill_styles[i].b) << "/255.0f, "
							   << "255/255.0f }," << endl;
					
					current_color += 1;
					
					break;
				}
				
				case FILL_GRAD_LINEAR:
				case FILL_GRAD_RADIAL:
				{
					MATRIX matrix;
					parseMatrix(matrix);
					
					recompileMatrix(matrix, uninv_mat_data);
					current_uninv += 1;
					
					fill_data.clearFields();
					fill_data.setFieldCount(1);
					
					fill_data.configureNextField(SWF_FIELD_UI8);
					
					fill_data.parseFields(cur_pos);
					
					// TODO: implement other spread and interpolation modes
					
					fill_styles[i].gradient.spread_mode = (u8) ((fill_data.fields[0].value & 0b11000000) >> 6);
					fill_styles[i].gradient.interpolation_mode = (u8) ((fill_data.fields[0].value & 0b00110000) >> 4);
					fill_styles[i].gradient.num_grads = (u8) (fill_data.fields[0].value & 0b00001111);
					
					for (int j = 0; j < fill_styles[i].gradient.num_grads; ++j)
					{
						fill_data.clearFields();
						fill_data.setFieldCount(1);
						
						fill_data.configureNextField(SWF_FIELD_UI8);
						
						fill_data.parseFields(cur_pos);
						
						fill_styles[i].gradient.records[j].ratio = (u8) fill_data.fields[0].value;
						
						RGB.parseFields(cur_pos);
						
						fill_styles[i].gradient.records[j].r = (u8) RGB.fields[0].value;
						fill_styles[i].gradient.records[j].g = (u8) RGB.fields[1].value;
						fill_styles[i].gradient.records[j].b = (u8) RGB.fields[2].value;
						
						if (j == 0)
						{
							continue;
						}
						
						GradientRecord& last_grad = fill_styles[i].gradient.records[j - 1];
						GradientRecord& grad = fill_styles[i].gradient.records[j];
						
						for (u8 ratio = last_grad.ratio; ratio < grad.ratio; ++ratio)
						{
							float ratio_diff = (float) (grad.ratio - last_grad.ratio);
							float t = (ratio - last_grad.ratio)/ratio_diff;
							
							u8 r = rgbLerp(last_grad.r, grad.r, t);
							u8 g = rgbLerp(last_grad.g, grad.g, t);
							u8 b = rgbLerp(last_grad.b, grad.b, t);
							
							gradient_data << "\t" << "{ "
										  << to_string(r) << ", "
										  << to_string(g) << ", "
										  << to_string(b) << ", "
										  << "255 },"
										  << endl;
						}
						
						if (grad.ratio == 255)
						{
							float ratio_diff = (float) (grad.ratio - last_grad.ratio);
							float t = (255 - last_grad.ratio)/ratio_diff;
							
							u8 r = rgbLerp(last_grad.r, grad.r, t);
							u8 g = rgbLerp(last_grad.g, grad.g, t);
							u8 b = rgbLerp(last_grad.b, grad.b, t);
							
							gradient_data << "\t" << "{ "
										  << to_string(r) << ", "
										  << to_string(g) << ", "
										  << to_string(b) << ", "
										  << "255 },"
										  << endl;
						}
					}
					
					fill_styles[i].index = current_gradient;
					
					current_gradient += 1;
					
					break;
				}
				
				case FILL_BITMAP_CLIPPED:
				{
					fill_data.clearFields();
					fill_data.setFieldCount(1);
					
					fill_data.configureNextField(SWF_FIELD_UI16);
					
					fill_data.parseFields(cur_pos);
					
					u16 char_id = (u16) fill_data.fields[0].value;
					
					fill_styles[i].index = ((current_uninv & 0xFFFF) << 16) | char_id_to_bitmap_id[char_id];
					
					MATRIX matrix;
					parseMatrix(matrix);
					
					recompileMatrix(matrix, uninv_mat_data);
					current_uninv += 1;
					
					break;
				}
			}
		}
		
		return fill_styles;
	}
	
	LineStyle* SWF::parseLineStyles(u16 line_style_count)
	{
		SWFTag line_data;
		
		LineStyle* line_styles = new LineStyle[line_style_count];
		
		for (u16 i = 0; i < line_style_count; ++i)
		{
			line_data.clearFields();
			line_data.setFieldCount(4);
			
			line_data.configureNextField(SWF_FIELD_UI16, 16);
			line_data.configureNextField(SWF_FIELD_UI8, 8);
			line_data.configureNextField(SWF_FIELD_UI8, 8);
			line_data.configureNextField(SWF_FIELD_UI8, 8);
			
			line_data.parseFields(cur_pos);
			
			line_styles[i].width = (u16) line_data.fields[0].value;
			
			line_styles[i].r = (u8) line_data.fields[1].value;
			line_styles[i].g = (u8) line_data.fields[2].value;
			line_styles[i].b = (u8) line_data.fields[3].value;
			
			line_styles[i].index = current_color;
			
			color_data << "\t" << "{ "
					   << to_string(line_styles[i].r) << "/255.0f, "
					   << to_string(line_styles[i].g) << "/255.0f, "
					   << to_string(line_styles[i].b) << "/255.0f, "
					   << "255/255.0f }," << endl;
			
			current_color += 1;
		}
		
		return line_styles;
	}
	
	void SWF::interpretShape(Context& context, SWFTag& shape_tag)
	{
		// TODO: DefineShape3
		// TODO: DefineShape4
		
		bool is_font = shape_tag.code == SWF_TAG_DEFINE_FONT;
		
		switch (shape_tag.code)
		{
			case SWF_TAG_DEFINE_SHAPE:
			case SWF_TAG_DEFINE_SHAPE_2:
			case SWF_TAG_DEFINE_FONT:
			{
				u16 shape_id;
				u16 fill_style_count;
				std::vector<FillStyle*> all_fill_styles;
				u16 line_style_count;
				std::vector<LineStyle*> all_line_styles;
				
				if (!is_font)
				{
					shape_tag.clearFields();
					shape_tag.setFieldCount(6);
					
					shape_tag.configureNextField(SWF_FIELD_UI16, 16);
					shape_tag.configureNextField(SWF_FIELD_UB, 5, true);
					shape_tag.configureNextField(SWF_FIELD_SB, 0);
					shape_tag.configureNextField(SWF_FIELD_SB, 0);
					shape_tag.configureNextField(SWF_FIELD_SB, 0);
					shape_tag.configureNextField(SWF_FIELD_SB, 0);
					
					shape_tag.parseFields(cur_pos);
					
					shape_id = (u16) shape_tag.fields[0].value;
					
					// FILLSTYLEARRAY
					shape_tag.clearFields();
					shape_tag.setFieldCount(1);
					
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
					
					fill_style_count = (u8) shape_tag.fields[0].value;
					
					if (fill_style_count == 0xFF)
					{
						shape_tag.clearFields();
						
						shape_tag.configureNextField(SWF_FIELD_UI16, 16);
						
						shape_tag.parseFields(cur_pos);
						
						fill_style_count = (u16) shape_tag.fields[0].value;
					}
					
					all_fill_styles.push_back(parseFillStyles(fill_style_count));
					
					// LINESTYLEARRAY
					shape_tag.clearFields();
					shape_tag.setFieldCount(1);
					
					shape_tag.configureNextField(SWF_FIELD_UI8, 8);
					
					shape_tag.parseFields(cur_pos);
					
					line_style_count = (u8) shape_tag.fields[0].value;
					
					if (line_style_count == 0xFF)
					{
						shape_tag.clearFields();
						
						shape_tag.configureNextField(SWF_FIELD_UI16, 16);
						
						shape_tag.parseFields(cur_pos);
						
						line_style_count = (u16) shape_tag.fields[0].value;
					}
					
					all_line_styles.push_back(parseLineStyles(line_style_count));
				}
				
				else
				{
					fill_style_count = 1;
					line_style_count = 0;
					
					FillStyle* fill_style = new FillStyle[fill_style_count];
					
					all_fill_styles.push_back(fill_style);
					
					fill_style->r = 0xFF;
					fill_style->g = 0xFF;
					fill_style->b = 0xFF;
					
					fill_style->type = 0x00;
					fill_style->index = current_color;
					
					color_data << "\t" << "{ "
							   << to_string(fill_style->r) << "/255.0f, "
							   << to_string(fill_style->g) << "/255.0f, "
							   << to_string(fill_style->b) << "/255.0f, "
							   << "255/255.0f }," << endl;
					
					current_color += 1;
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
					bool state_line_style = !is_font && (state_flags & 0b01000) != 0;
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
						
						all_fill_styles.push_back(parseFillStyles(fill_style_count));
						
						current_fill_style_list += 1;
						
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
						
						all_line_styles.push_back(parseLineStyles(line_style_count));
						
						current_line_style_list += 1;
						
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
						if (paths.size() > 0 && paths.back().verts.size() == 1)
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
					
					last_line_style = line_style;
				}
				
				if (cur_byte_bits_left != 8)
				{
					cur_pos += 1;
				}
				
				if (current_path == nullptr)
				{
					return;
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
							if (shapes.back().verts.size() > 0 &&
								shapes.back().verts.back().x == cycle[j].verts[k].x &&
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
				
				size_t tris_size = 0;
				
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
								float x_f = (float) t.verts[j].x;
								float y_f = (float) (FRAME_HEIGHT - t.verts[j].y);
								
								shape_data << "\t" << "{ "
										   << std::hex << std::uppercase
										   << "0x" << VAL(u32, &x_f) << ", "
										   << "0x" << VAL(u32, &y_f) << ", "
										   << "0x" << (u32) all_fill_styles[shapes[i].fill_style_list][shapes[i].inner_fill - 1].type << ", "
										   << "0x" << (u32) all_fill_styles[shapes[i].fill_style_list][shapes[i].inner_fill - 1].index
										   << " }," << endl;
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
								float x_f = (float) t.verts[j].x;
								float y_f = (float) (FRAME_HEIGHT - t.verts[j].y);
								
								shape_data << "\t" << "{ "
										   << std::hex << std::uppercase
										   << "0x" << VAL(u32, &x_f) << ", "
										   << "0x" << VAL(u32, &y_f) << ", "
										   << "0x00, "
										   << "0x" << (u32) line_style.index
										   << " }," << endl;
							}
						}
					}
				}
				
				context.tag_main << "\t" << "tagDefineShape(" << to_string(shape_id) << ", " << to_string(3*current_tri) << ", " << to_string(3*tris_size) << ");" << endl;
				
				current_tri += tris_size;
				
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
	
	void drawLineJoin(const Vertex& a, const Vertex& b, const Vertex& c, u16 halfwidth, std::vector<Tri>& tris)
	{
		Vertex vec_a_b;
		vec_a_b.x = b.x - a.x;
		vec_a_b.y = b.y - a.y;
		
		Vertex vec_b_c;
		vec_b_c.x = c.x - b.x;
		vec_b_c.y = c.y - b.y;
		
		s32 cross = CROSS(vec_a_b, vec_b_c);
		
		double offset = (cross < 0) ? M_PI/2 : -M_PI/2;
		
		double angle_a_b = atan2(vec_a_b.y, vec_a_b.x) + offset;
		double angle_b_c = atan2(vec_b_c.y, vec_b_c.x) + offset;
		
		int num_midpoints = 5;
		
		double start_angle = (angle_a_b < angle_b_c) ? angle_a_b : angle_b_c;
		double end_angle = (angle_a_b < angle_b_c) ? angle_b_c : angle_a_b;
		
		double angle_delta = (end_angle - start_angle)/num_midpoints;
		
		Vertex last_point;
		last_point.x = (s32) std::round(b.x + halfwidth*cos(start_angle));
		last_point.y = (s32) std::round(b.y + halfwidth*sin(start_angle));
		
		Tri t;
		t.verts[0] = b;
		
		for (double current_angle = start_angle + angle_delta; current_angle < end_angle; current_angle += angle_delta)
		{
			t.verts[1] = last_point;
			
			t.verts[2].x = (s32) std::round(b.x + halfwidth*cos(current_angle));
			t.verts[2].y = (s32) std::round(b.y + halfwidth*sin(current_angle));
			
			tris.push_back(t);
			
			last_point = t.verts[2];
		}
		
		t.verts[1] = last_point;
		
		t.verts[2].x = (s32) std::round(b.x + halfwidth*cos(end_angle));
		t.verts[2].y = (s32) std::round(b.y + halfwidth*sin(end_angle));
		
		tris.push_back(t);
	}
	
	void drawLineCap(const Vertex& a, const Vertex& b, u16 halfwidth, std::vector<Tri>& tris)
	{
		Vertex vec_a_b;
		vec_a_b.x = b.x - a.x;
		vec_a_b.y = b.y - a.y;
		
		double angle_a_b = atan2(vec_a_b.y, vec_a_b.x);
		
		int num_midpoints = 5;
		
		double start_angle = angle_a_b + M_PI/2.0;
		double end_angle = start_angle + M_PI;
		
		double angle_delta = (end_angle - start_angle)/num_midpoints;
		
		Vertex last_point;
		last_point.x = (s32) std::round(a.x + halfwidth*cos(start_angle));
		last_point.y = (s32) std::round(a.y + halfwidth*sin(start_angle));
		
		Tri t;
		t.verts[0] = a;
		
		for (double current_angle = start_angle + angle_delta; current_angle < end_angle; current_angle += angle_delta)
		{
			t.verts[1] = last_point;
			
			t.verts[2].x = (s32) std::round(a.x + halfwidth*cos(current_angle));
			t.verts[2].y = (s32) std::round(a.y + halfwidth*sin(current_angle));
			
			tris.push_back(t);
			
			last_point = t.verts[2];
		}
		
		t.verts[1] = last_point;
		
		t.verts[2].x = (s32) std::round(a.x + halfwidth*cos(end_angle));
		t.verts[2].y = (s32) std::round(a.y + halfwidth*sin(end_angle));
		
		tris.push_back(t);
	}
	
	void SWF::drawLines(const Path& path, u16 width, std::vector<Tri>& tris)
	{
		if (width != 0 && width < 20)
		{
			width = 20;
		}
		
		else if (width == 0)
		{
			return;
		}
		
		u16 halfwidth = width/2;
		
		Vertex last_v = path.verts[0];
		
		for (size_t i = 1; i < path.verts.size(); ++i)
		{
			const Vertex& v = path.verts[i];
			
			Tri t;
			
			double angle = atan2(v.y - last_v.y, v.x - last_v.x) - M_PI/2.0;
			
			if (i > 1)
			{
				const Vertex& last_last_v = path.verts[i - 2];
				
				drawLineJoin(last_last_v, last_v, v, halfwidth, tris);
			}
			
			t.verts[0].x = (s32) std::round(last_v.x + halfwidth*cos(angle));
			t.verts[0].y = (s32) std::round(last_v.y + halfwidth*sin(angle));
			
			t.verts[2].x = (s32) std::round(v.x + halfwidth*cos(angle));
			t.verts[2].y = (s32) std::round(v.y + halfwidth*sin(angle));
			
			angle += M_PI;
			
			t.verts[1].x = (s32) std::round(last_v.x + halfwidth*cos(angle));
			t.verts[1].y = (s32) std::round(last_v.y + halfwidth*sin(angle));
			
			tris.push_back(t);
			
			t.verts[0].x = (s32) std::round(v.x + halfwidth*cos(angle));
			t.verts[0].y = (s32) std::round(v.y + halfwidth*sin(angle));
			
			tris.push_back(t);
			
			last_v = v;
		}
		
		if (path.verts.size() > 2 &&
			path.verts.back().x == path.verts[0].x &&
			path.verts.back().y == path.verts[0].y)
		{
			const Vertex& a = path.verts[path.verts.size() - 2];
			const Vertex& b = path.verts[0];
			const Vertex& c = path.verts[1];
			
			drawLineJoin(a, b, c, halfwidth, tris);
		}
		
		else
		{
			drawLineCap(path.verts[0], path.verts[1], halfwidth, tris);
			drawLineCap(path.verts.back(), path.verts[path.verts.size() - 2], halfwidth, tris);
		}
	}
};