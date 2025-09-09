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
	void recompile(Context& context)
	{
		printf("\n");
		
		if (!fs::exists(context.output_tags_folder))
		{
			fs::create_directory(context.output_tags_folder);
		}
		
		if (!fs::exists(context.output_scripts_folder))
		{
			fs::create_directory(context.output_scripts_folder);
		}
		
		context.output_tags_folder = string("") + context.output_tags_folder + ((char) fs::path::preferred_separator);
		context.output_scripts_folder = string("") + context.output_scripts_folder + ((char) fs::path::preferred_separator);
		
		context.tag_main = ofstream(string("") + context.output_tags_folder + "tagMain.c", ios_base::out);
		
		context.constants_header = ofstream(string("") + context.output_tags_folder + "constants.h", ios_base::out);
		context.constants_header << "#pragma once" << endl << endl;
		
		context.out_draws = ofstream(string("") + context.output_tags_folder + "draws.c", ios_base::out);
		context.out_draws << "#include \"recomp.h\"" << endl
						  << "#include \"draws.h\"" << endl;
		
		context.out_draws_header = ofstream(string("") + context.output_tags_folder + "draws.h", ios_base::out);
		context.out_draws_header << "#pragma once" << endl;
		
		SWF swf = SWF(context);
		
		swf.parseAllTags(context);
		
		context.tag_main.close();
		context.constants_header.close();
		context.out_draws.close();
		context.out_draws_header.close();
	}
};