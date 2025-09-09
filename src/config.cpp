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
	
	void Config::parse_file(string path)
	{
		tbl = toml::parse_file(path);
		
		string_view swf_path_view = parse_string_view("path_to_swf");
		string_view output_tags_folder_view = parse_string_view("output_tags_folder");
		string_view output_scripts_folder_view = parse_string_view("output_scripts_folder");
		
		swf_path = string(swf_path_view);
		output_tags_folder = string(output_tags_folder_view);
		output_scripts_folder = string(output_scripts_folder_view);
	}
	
	string_view Config::parse_string_view(string key)
	{
		string_view view = tbl["input"][key].value_or(""sv);
		
		if (view == "")
		{
			EXC_ARG("Error: field %s in toml must be present and non-empty\n", key.c_str());
		}
		
		return view;
	}
}