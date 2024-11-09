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
		
		string output_tag_main = string("") + output_tags_folder + ((char) fs::path::preferred_separator) + "tagMain.c";  // Gross.
		ofstream tag_main(output_tag_main, ios_base::out);
		
		tag_main << "#include <recomp.h>" << endl << endl
				 << "void tagMain()" << endl
				 << "{" << endl;
		
		while (swf.parseTag(tag_main));
		
		tag_main << "}" << endl;
	}
};