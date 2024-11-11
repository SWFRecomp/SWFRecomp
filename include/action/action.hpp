#pragma once

#include <fstream>

#include <common.h>
#include <stackvalue.hpp>

using std::ofstream;

namespace SWFRecomp
{
	enum SWFActionType
	{
		SWF_ACTION_END_OF_ACTIONS = 0x00,
		SWF_ACTION_TRACE = 0x26,
		SWF_ACTION_CONSTANT_POOL = 0x88,
		SWF_ACTION_PUSH = 0x96
	};
	
	class SWFAction
	{
	public:
		StackValue* stack;
		size_t stack_size;
		size_t sp;
		
		SWFAction();
		
		char* parseActions(char* action_buffer, ofstream& out_script);
		void push(StackValueType type, u64 value);
	};
};