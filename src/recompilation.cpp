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
		
		if (!fs::exists(context.config.output_tags_folder))
		{
			fs::create_directory(context.config.output_tags_folder);
		}
		
		if (!fs::exists(context.config.output_scripts_folder))
		{
			fs::create_directory(context.config.output_scripts_folder);
		}
		
		context.config.output_tags_folder = string("") + context.config.output_tags_folder + ((char) fs::path::preferred_separator);
		context.config.output_scripts_folder = string("") + context.config.output_scripts_folder + ((char) fs::path::preferred_separator);
		
		context.tag_main = ofstream(string("") + context.config.output_tags_folder + "tagMain.c", ios_base::out);
		
		context.constants = ofstream(string("") + context.config.output_tags_folder + "constants.c", ios_base::out);
		
		context.constants_header = ofstream(string("") + context.config.output_tags_folder + "constants.h", ios_base::out);
		context.constants_header << "#pragma once" << endl << endl;
		
		context.out_draws = ofstream(string("") + context.config.output_tags_folder + "draws.c", ios_base::out);
		context.out_draws << "#include \"recomp.h\"" << endl
						  << "#include \"draws.h\"";
		
		context.out_draws_header = ofstream(string("") + context.config.output_tags_folder + "draws.h", ios_base::out);
		context.out_draws_header << "#pragma once" << endl;
		
		SWF swf = SWF(context, context.config.swf_path);
		
		swf.openSWF(context);
		
		std::string prelude_swf_path = context.config.prelude_swf_path;
		
		if (prelude_swf_path != "")
		{
			context.prelude = true;
			
			SWF prelude = SWF(context, prelude_swf_path);
			
			swf.parsePrelude(context, prelude.cur_pos);
			
			for (size_t i = 0; i < swf.next_init_script_i; ++i)
			{
				context.tag_init << endl
								 << "\tinit_script_" << to_string(i) << "(app_context);";
			}
		}
		
		context.prelude = false;
		
		swf.parseAllTags(context);
		
		swf.closeSWF(context);
		
		context.tag_main.close();
		context.constants.close();
		context.constants_header.close();
		context.out_draws.close();
		context.out_draws_header.close();
	}
};