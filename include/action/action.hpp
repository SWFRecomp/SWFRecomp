#pragma once

#include <fstream>

#include <common.h>
#include <stackvalue.hpp>

using std::string;
using std::ofstream;
using std::ostream;

namespace SWFRecomp
{
	enum SWFActionType
	{
		SWF_ACTION_END_OF_ACTIONS = 0x00,
		SWF_ACTION_POP = 0x17,
		SWF_ACTION_GET_VARIABLE = 0x1C,
		SWF_ACTION_SET_VARIABLE = 0x1D,
		SWF_ACTION_TRACE = 0x26,
		SWF_ACTION_CONSTANT_POOL = 0x88,
		SWF_ACTION_PUSH = 0x96
	};
	
	class SWFAction
	{
	public:
		size_t next_static_i;
		
		SWFAction();
		
		char* parseActions(char* action_buffer, ofstream& out_script);
		void declareVariable(char* var_name, ostream& out_script);
		void createStaticString(char* str, ostream& out_script);
	};
};