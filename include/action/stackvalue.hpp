#pragma once

namespace SWFRecomp
{
	enum StackValueType
	{
		STACK_VALUE_STRING = 0,
		STACK_VALUE_F32 = 1
	};
	
	class StackValue
	{
	public:
		StackValueType type;
		u64 value;
	};
};