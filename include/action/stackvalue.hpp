#pragma once

namespace SWFRecomp
{
	enum ActionStackValueType
	{
		ACTION_STACK_VALUE_STRING = 0,
		ACTION_STACK_VALUE_F32 = 1,
		ACTION_STACK_VALUE_UNKNOWN = 10,
		ACTION_STACK_VALUE_VARIABLE = 11
	};
	
	class ActionStackValue
	{
	public:
		ActionStackValueType type;
		u64 value;
	};
};