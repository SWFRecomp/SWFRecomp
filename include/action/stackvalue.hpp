#pragma once

#include <common.h>

namespace SWFRecomp
{
	enum ActionStackValueType
	{
		ACTION_STACK_VALUE_STRING = 0,
		ACTION_STACK_VALUE_F32 = 1,
		ACTION_STACK_VALUE_NULL = 2,
		ACTION_STACK_VALUE_UNDEFINED = 3,
		ACTION_STACK_VALUE_REGISTER = 4,
		ACTION_STACK_VALUE_F64 = 6,
		ACTION_STACK_VALUE_INT = 7,
		ACTION_STACK_VALUE_CONST8 = 8,
		ACTION_STACK_VALUE_UNSET = 15
	};
	
	class ActionStackValue
	{
	public:
		ActionStackValueType type;
		u64 value;
	};
};