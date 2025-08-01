#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

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
	
	struct Node;
	
	struct Path
	{
		std::vector<Vertex> verts;
		u32 fill_style_list;
		u32 line_style_list;
		u32 fill_styles[2];
		u32 line_style;
		bool self_closed;
		bool backward;
		Node* front;
		Node* back;
	};
	
	struct Node
	{
		Path* path;
		std::vector<Node*> neighbors;
		bool used;
	};
	
	struct Shape
	{
		std::vector<Vertex> verts;
		std::vector<Shape*> holes;
		Vertex min;
		Vertex max;
		bool got_min_max;
		u32 fill_style_list;
		u32 line_style_list;
		u32 inner_fill;
		u32 outer_fill;
		bool fill_right;
		bool closed;
		bool hole;
		bool invalid;
	};
	
	struct FillStyle
	{
		u8 type;
		u8 r;
		u8 g;
		u8 b;
	};
	
	struct LineStyle
	{
		u16 width;
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
		bool isInShape(const Vertex& v, const Shape* shape);
		void addCurvedEdge(Path* path, Vertex current, Vertex control, Vertex anchor, u32 passes);
		void processShape(Shape& shape, u32* fill_styles);
		void constructEdges(std::vector<Path>& paths, std::vector<Node>& nodes);
		void johnson(std::vector<Node>& nodes, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths);
		void fillShape(Shape& shape, std::vector<Tri>& tris);
		void drawLines(const Path& path, u16 width, std::vector<Tri>& tris);
	};
};