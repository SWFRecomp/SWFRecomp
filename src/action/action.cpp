#include <cstring>

#include <action.hpp>

#define VAL(type, x) *((type*) x)

using std::endl;

namespace SWFRecomp
{
	SWFAction::SWFAction() : sp(0)
	{
		stack_size = 256;
		stack = new StackValue[stack_size];
	}
	
	char* SWFAction::parseActions(char* action_buffer, ofstream& out_script)
	{
		u8 code = SWF_ACTION_CONSTANT_POOL;
		u16 length;
		
		while (code != SWF_ACTION_END_OF_ACTIONS)
		{
			code = action_buffer[0];
			action_buffer += 1;
			
			length = 0;
			
			if ((code & 0b10000000) != 0)
			{
				length = VAL(u16, action_buffer);
				action_buffer += 2;
			}
			
			switch (code)
			{
				case SWF_ACTION_END_OF_ACTIONS:
				{
					break;
				}
				
				case SWF_ACTION_TRACE:
				{
					sp -= 1;
					out_script << "\t" << "actionTrace(\"" << ((char*) stack[sp].value) << "\\n\");" << endl;
					
					break;
				}
				
				case SWF_ACTION_CONSTANT_POOL:
				{
					action_buffer += length;
					
					break;
				}
				
				case SWF_ACTION_PUSH:
				{
					StackValueType push_type = (StackValueType) action_buffer[0];
					action_buffer += 1;
					
					u64 push_value;
					
					switch (push_type)
					{
						case STACK_VALUE_STRING:
						{
							push_value = (u64) action_buffer;
							size_t push_length = strnlen((char*) push_value, 1024) + 1;
							
							if (push_length == 1024)
							{
								EXC("You can't be serious.\n");
							}
							
							action_buffer += push_length;
							
							break;
						}
					}
					
					push(push_type, push_value);
					
					break;
				}
				
				default:
				{
					EXC_ARG("Unimplemented action 0x%02X\n", code);
					
					break;
				}
			}
		}
		
		return action_buffer;
	}
	
	void SWFAction::push(StackValueType type, u64 value)
	{
		if (sp >= stack_size)
		{
			stack_size <<= 1;
			StackValue* new_stack = new StackValue[stack_size];
			
			for (size_t i = 0; i < (stack_size >> 1); ++i)
			{
				new_stack[i].type = stack[i].type;
				new_stack[i].value = stack[i].value;
			}
			
			delete[] stack;
			stack = new_stack;
		}
		
		stack[sp].type = type;
		stack[sp].value = value;
		sp += 1;
	}
};