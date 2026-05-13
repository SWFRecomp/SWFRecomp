#pragma once

#include <fstream>
#include <vector>

#include <config.hpp>

using std::string;
using std::ofstream;
using std::vector;

namespace SWFRecomp
{
	struct Context
	{
		Config config;
		
		bool prelude;
		
		std::stringstream tag_init;
		
		ofstream tag_main;
		ofstream constants;
		ofstream constants_header;
		ofstream out_script_header;
		ofstream out_script_defs;
		ofstream out_script_decls;
		vector<ofstream> out_funcs;
		size_t num_files;
		ofstream out_draws;
		ofstream out_draws_header;
	};
};