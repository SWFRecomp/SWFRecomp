#include <string>
#include <filesystem>
#include <fstream>

#include <common.h>
#include <swf.hpp>

using std::string;
using std::to_string;

using std::ofstream;
using std::ios_base;
using std::endl;

namespace fs = std::filesystem;

namespace SWFRecomp
{
	void recompile(const char* swf_path, const char* output_tags_folder, const char* output_scripts_folder)
	{
		SWF swf = SWF(swf_path);
		
		printf("\n");
		
		if (!fs::exists(output_tags_folder))
		{
			fs::create_directory(output_tags_folder);
		}
		
		if (!fs::exists(output_scripts_folder))
		{
			fs::create_directory(output_scripts_folder);
		}
		
		string output_tag_main = string("") + output_tags_folder + ((char) fs::path::preferred_separator) + "tagMain.c";  // Gross.
		ofstream tag_main(output_tag_main, ios_base::out);
		
		string output_scripts_folder_slashed = string("") + output_scripts_folder + ((char) fs::path::preferred_separator);
		swf.parseAllTags(tag_main, output_scripts_folder_slashed);
		
		tag_main << "}";
	}
};