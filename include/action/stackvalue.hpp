#pragma once

namespace SWFRecomp
{
	enum ActionStackValueType
	{
		ACTION_STACK_VALUE_STRING = 0,
		ACTION_STACK_VALUE_F32 = 1
	};
	
	class ActionStackValue
	{
	public:
		ActionStackValueType type;
		u64 value;
	};
};