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
		
		char* loadOtherData(char* swf_buffer);
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
		
		bool parseTag(ofstream& tag_main, const string& output_scripts_folder);
		void parseAllTags(ofstream& tag_main, const string& output_scripts_folder);
		void interpretTag(SWFTag& tag, ofstream& tag_main, const string& output_scripts_folder);
	};
};