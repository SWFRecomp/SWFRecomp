#pragma once

#include <toml++/toml.hpp>

#include <common.h>

namespace SWFRecomp
{
	class Config
	{
	public:
		toml::table tbl;
		std::string swf_path;
		std::string output_tags_folder;
		std::string output_scripts_folder;
		
		Config();
		void parse_file(std::string path);
		std::string_view parse_string_view(std::string key);
	};
};