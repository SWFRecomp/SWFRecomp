#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>

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
		SWF_ACTION_DELETE = 0x3A,
		SWF_ACTION_DELETE2 = 0x3B,
		SWF_ACTION_DEFINE_LOCAL = 0x3C,
		SWF_ACTION_CALL_FUNCTION = 0x3D,
		SWF_ACTION_RETURN = 0x3E,
		SWF_ACTION_MODULO = 0x3F,
		SWF_ACTION_NEW_OBJECT = 0x40,
		SWF_ACTION_DECLARE_LOCAL = 0x41,
		SWF_ACTION_INIT_ARRAY = 0x42,
		SWF_ACTION_INIT_OBJECT = 0x43,
		SWF_ACTION_TYPEOF = 0x44,
		SWF_ACTION_ENUMERATE = 0x46,
		SWF_ACTION_ADD2 = 0x47,
		SWF_ACTION_LESS2 = 0x48,
		SWF_ACTION_EQUALS2 = 0x49,
		SWF_ACTION_TO_NUMBER = 0x4A,
		SWF_ACTION_TO_STRING = 0x4B,
		SWF_ACTION_DUPLICATE = 0x4C,
		SWF_ACTION_STACK_SWAP = 0x4D,
		SWF_ACTION_GET_MEMBER = 0x4E,
		SWF_ACTION_SET_MEMBER = 0x4F,
		SWF_ACTION_INCREMENT = 0x50,
		SWF_ACTION_DECREMENT = 0x51,
		SWF_ACTION_CALL_METHOD = 0x52,
		SWF_ACTION_NEW_METHOD = 0x53,
		SWF_ACTION_INSTANCEOF = 0x54,
		SWF_ACTION_ENUMERATE2 = 0x55,
		SWF_ACTION_STRICT_EQUALS = 0x66,
		SWF_ACTION_GREATER = 0x67,
		SWF_ACTION_EXTENDS = 0x69,
		SWF_ACTION_STORE_REGISTER = 0x87,
		SWF_ACTION_CONSTANT_POOL = 0x88,
		SWF_ACTION_DEFINE_FUNCTION2 = 0x8E,
		SWF_ACTION_PUSH = 0x96,
		SWF_ACTION_JUMP = 0x99,
		SWF_ACTION_DEFINE_FUNCTION = 0x9B,
		SWF_ACTION_IF = 0x9D
	};

	class SWFAction
	{
	public:
		size_t next_str_i;
		size_t func_counter;
		std::unordered_map<std::string, size_t> string_to_id;  // Track declared strings for deduplication
		std::vector<size_t> constant_pool;  // Maps constant pool index to string ID

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