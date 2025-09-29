#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <common.h>
#include <tag.hpp>
#include <action.hpp>

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
	
	struct MATRIX
	{
		float scale_x;
		float scale_y;
		float rotateskew_0;
		float rotateskew_1;
		s32 translate_x;
		s32 translate_y;
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
	
	struct GradientRecord
	{
		u8 ratio;
		u8 r;
		u8 g;
		u8 b;
	};
	
	struct Gradient
	{
		u8 spread_mode;
		u8 interpolation_mode;
		u8 num_grads;
		GradientRecord records[15];
	};
	
	enum FillType
	{
		FILL_SOLID = 0x00,
		FILL_GRAD_LINEAR = 0x10,
		FILL_GRAD_RADIAL = 0x12,
		FILL_GRAD_FOCAL = 0x13,
		FILL_BITMAP_REPEAT = 0x40,
		FILL_BITMAP_CLIPPED = 0x41,
		FILL_BITMAP_REPEAT_NONSMOOTH = 0x42,
		FILL_BITMAP_CLIPPED_NONSMOOTH = 0x43,
	};
	
	struct FillStyle
	{
		u8 type;
		size_t index;
		u8 r;
		u8 g;
		u8 b;
		Gradient gradient;
	};
	
	struct LineStyle
	{
		u16 width;
		size_t index;
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
		size_t num_finished_tags;
		size_t next_frame_i;
		bool another_frame;
		size_t next_script_i;
		size_t last_queued_script;
		
		std::stringstream shape_data;
		size_t current_tri;
		std::stringstream transform_data;
		size_t current_transform;
		std::stringstream color_data;
		size_t current_color;
		std::stringstream uninv_mat_data;
		size_t current_uninv;
		std::stringstream gradient_data;
		size_t current_gradient;
		
		u8* jpeg_tables;
		size_t jpeg_tables_size;
		
		SWFAction action;
		
		SWFTag RGB;
		
		SWF();
		SWF(Context& context);
		
		void parseMatrix(MATRIX& matrix_out);
		void parseAllTags(Context& context);
		void interpretTag(Context& context, SWFTag& tag);
		FillStyle* parseFillStyles(u16 fill_style_count);
		LineStyle* parseLineStyles(u16 line_style_count);
		void interpretShape(Context& context, SWFTag& shape_tag);
		bool isInShape(const Vertex& v, const Shape* shape);
		void addCurvedEdge(Path* path, Vertex current, Vertex control, Vertex anchor, u32 passes);
		void processShape(Shape& shape, u32* fill_styles);
		void constructEdges(std::vector<Path>& paths, std::vector<Node>& nodes);
		void johnson(std::vector<Node>& nodes, std::vector<Path>& path_stack, std::unordered_map<Node*, bool>& blocked, std::unordered_map<Node*, std::vector<Node*>>& blocked_map, std::vector<std::vector<Path>>& closed_paths);
		void fillShape(Shape& shape, std::vector<Tri>& tris);
		void drawLines(const Path& path, u16 width, std::vector<Tri>& tris);
	};
};