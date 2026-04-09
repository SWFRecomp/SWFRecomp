#include <iostream>

#include <config.hpp>

using std::string;
using std::string_view;

using namespace std::string_view_literals;

namespace SWFRecomp
{
	Config::Config()
	{
		
	}
	
	void Config::parseFile(string path)
	{
		tbl = toml::parse_file(path);
		
		string_view swf_path_view = parseStringView("path_to_swf");
		string_view output_tags_folder_view = parseStringView("output_tags_folder");
		string_view output_scripts_folder_view = parseStringView("output_scripts_folder");
		string_view funcs_per_file_view = parseStringViewOrEmpty("funcs_per_file");
		
		swf_path = string(swf_path_view);
		output_tags_folder = string(output_tags_folder_view);
		output_scripts_folder = string(output_scripts_folder_view);
		
		std::string funcs_per_file_str = string(funcs_per_file_view);
		
		funcs_per_file = (funcs_per_file_str == "") ? FUNCS_PER_FILE_DEFAULT : std::stoi(funcs_per_file_str);
	}
	
	string_view Config::parseStringView(string key)
	{
		string_view view = tbl["input"][key].value_or(""sv);
		
		if (view == "")
		{
			EXC_ARG("Error: field %s in toml must be present and non-empty\n", key.c_str());
		}
		
		return view;
	}
	
	string_view Config::parseStringViewOrEmpty(string key)
	{
		return tbl["input"][key].value_or(""sv);
	}
}