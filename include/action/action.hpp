#pragma once

#include <fstream>
#include <map>

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
		SWF_ACTION_STOP = 0x07,
		SWF_ACTION_ADD = 0x0A,
		SWF_ACTION_SUBTRACT = 0x0B,
		SWF_ACTION_MULTIPLY = 0x0C,
		SWF_ACTION_DIVIDE = 0x0D,
		SWF_ACTION_EQUALS = 0x0E,
		SWF_ACTION_LESS = 0x0F,
		SWF_ACTION_AND = 0x10,
		SWF_ACTION_OR = 0x11,
		SWF_ACTION_NOT = 0x12,
		SWF_ACTION_STRING_EQUALS = 0x13,
		SWF_ACTION_STRING_LENGTH = 0x14,
		SWF_ACTION_POP = 0x17,
		SWF_ACTION_GET_VARIABLE = 0x1C,
		SWF_ACTION_SET_VARIABLE = 0x1D,
		SWF_ACTION_STRING_ADD = 0x21,
		SWF_ACTION_TRACE = 0x26,
		SWF_ACTION_GET_TIME = 0x34,
		SWF_ACTION_CONSTANT_POOL = 0x88,
		SWF_ACTION_PUSH = 0x96,
		SWF_ACTION_JUMP = 0x99,
		SWF_ACTION_IF = 0x9D
	};
	
	class SWFAction
	{
	public:
		size_t next_str_i;
		std::map<std::string, size_t> string_to_id;  // Track declared strings for deduplication
		
		SWFAction();
		
		void parseActions(Context& context, char*& action_buffer, ofstream& out_script);
		void declareVariable(Context& context, char* var_name);
		void declareString(Context& context, char* str);
		void declareEmptyString(Context& context, size_t size);
		size_t getStringId(const char* str);  // Get ID for a previously declared string
		char actionCodeLookAhead(char* action_buffer, int lookAhead);
		size_t actionCodeLookAheadIndex(char* action_buffer, int lookAhead);
	};
};