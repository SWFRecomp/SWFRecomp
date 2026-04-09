#pragma once

#include <toml++/toml.hpp>

#include <common.h>

namespace SWFRecomp
{
	class Config
	{
	public:
		const size_t FUNCS_PER_FILE_DEFAULT = 50;
		
		toml::table tbl;
		std::string swf_path;
		std::string output_tags_folder;
		std::string output_scripts_folder;
		size_t funcs_per_file;
		
		Config();
		void parseFile(std::string path);
		std::string_view parseStringView(std::string key);
		std::string_view parseStringViewOrEmpty(std::string key);
	};
};