#pragma once

#include <fstream>
#include <string>

#include <common.h>
#include <tag.hpp>
#include <action.hpp>

using std::ofstream;
using std::string;

namespace SWFRecomp
{
	struct RECT
	{
		u8 nbits;
		s32 xmin;
		s32 xmax;
		s32 ymin;
		s32 ymax;
	};
	
	struct Vertex
	{
		s32 x;
		s32 y;
	};
	
	struct Tri
	{
		Vertex verts[3];
	};
	
	struct Path
	{
		std::vector<Vertex> verts;
		u32 fill_styles[2];
		bool read_style[2];
	};
	
	struct Shape
	{
		std::vector<Vertex> verts;
		Vertex min;
		Vertex max;
		bool got_min_max;
		u32 inner_fill;
		u32 outer_fill;
		int fill_right;
		bool closed;
		bool hole;
	};
	
	struct FillStyle
	{
		u8 type;
		u8 r;
		u8 g;
		u8 b;
	};
	
	class SWFHeader
	{
	public:
		u8 compression;
		u8 w;  // If not 'W', invalid SWF
		u8 s;  // If not 'S', invalid SWF
		
		u8 version;
		u32 file_length;
		RECT frame_size;
		u16 framerate;
		u16 frame_count;
		
		SWFHeader();
		SWFHeader(char* swf_buffer);
		
		void loadOtherData(char*& swf_buffer);
	};
	
	class SWF
	{
	public:
		SWFHeader header;
		char* swf_buffer;
		char* cur_pos;
		size_t next_frame_i;
		bool another_frame;
		size_t next_script_i;
		size_t last_queued_script;
		
		SWFAction action;
		
		SWFTag RGB;
		
		SWF();
		SWF(const char* swf_path);
		
		void parseAllTags(ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header, const string& output_scripts_folder);
		void interpretTag(SWFTag& tag, ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header, const string& output_scripts_folder);
		void interpretShape(SWFTag& shape_tag, ofstream& tag_main, ofstream& out_draws, ofstream& out_draws_header);
		void fillShape(std::vector<Vertex>& shape, std::vector<Tri>& tris, bool fill_right);
	};
};