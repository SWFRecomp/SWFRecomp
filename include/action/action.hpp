#pragma once

#include <fstream>
#include <unordered_map>

#include <common.h>
#include <stackvalue.hpp>

using std::unordered_map;
using std::string;
using std::ofstream;

namespace SWFRecomp
{
	enum SWFActionType
	{
		SWF_ACTION_END_OF_ACTIONS = 0x00,
		SWF_ACTION_GET_VARIABLE = 0x1C,
		SWF_ACTION_SET_VARIABLE = 0x1D,
		SWF_ACTION_TRACE = 0x26,
		SWF_ACTION_CONSTANT_POOL = 0x88,
		SWF_ACTION_PUSH = 0x96
	};
	
	class SWFAction
	{
	public:
		ActionStackValue* stack;
		size_t stack_size;
		size_t sp;
		unordered_map<string, bool> vars;
		size_t next_static_i;
		
		SWFAction();
		
		char* parseActions(char* action_buffer, ofstream& out_script);
		void push(ActionStackValueType type, u64 value);
	};
};