#include <cstring>
#include <string>
#include <iomanip>

#include <action.hpp>

#define VAL(type, x) *((type*) x)

using std::to_string;
using std::endl;

namespace SWFRecomp
{
	SWFAction::SWFAction() : stack_size(256), sp(0), next_static_i(0)
	{
		stack = new ActionStackValue[stack_size];
	}
	
	char* SWFAction::parseActions(char* action_buffer, ofstream& out_script)
	{
		SWFActionType code = SWF_ACTION_CONSTANT_POOL;
		u16 length;
		
		while (code != SWF_ACTION_END_OF_ACTIONS)
		{
			code = (SWFActionType) (u8) action_buffer[0];
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
				
				case SWF_ACTION_GET_VARIABLE:
				{
					stack[sp - 1].type = ACTION_STACK_VALUE_VARIABLE;
					
					break;
				}
				
				case SWF_ACTION_SET_VARIABLE:
				{
					sp -= 2;
					char* var_name = (char*) stack[sp].value;
					
					if (vars.count(var_name) == 0)
					{
						out_script << "\t" << "var " << var_name << ";" << endl;
						vars[var_name] = true;
					}
					
					switch (stack[sp + 1].type)
					{
						case ACTION_STACK_VALUE_STRING:
						{
							out_script << "\t" << "static const char* str_" << next_static_i << " = \"" << (char*) stack[sp + 1].value << "\";" << endl
									   << "\t" << string(var_name) << ".type = ACTION_STACK_VALUE_STRING;" << endl
									   << "\t" << string(var_name) << ".value = (u64) str_" << to_string(next_static_i) << ";" << endl;
							next_static_i += 1;
							
							break;
						}
						
						case ACTION_STACK_VALUE_F32:
						{
							out_script << "\t" << string(var_name) + ".type = ACTION_STACK_VALUE_F32;" << endl
									   << "\t" << string(var_name) + ".value = " << (VAL(u32, &stack[sp + 1].value)) << ";" << endl;
							
							break;
						}
						
						case ACTION_STACK_VALUE_UNKNOWN:
						{
							out_script << "stack[--sp].value;" << endl;
							
							break;
						}
					}
					
					break;
				}
				
				case SWF_ACTION_TRACE:
				{
					sp -= 1;
					
					switch (stack[sp].type)
					{
						case ACTION_STACK_VALUE_STRING:
						{
							out_script << "\t" << "actionTrace(\"" << ((char*) stack[sp].value) << "\");" << endl;
							break;
						}
						
						case ACTION_STACK_VALUE_F32:
						{
							out_script << "\t" << "actionTrace(\"" << std::setprecision(15) << (VAL(float, &stack[sp].value)) << "\");" << endl;
							break;
						}
						
						case ACTION_STACK_VALUE_VARIABLE:
						{
							out_script << "\t" << "actionTraceVar(" << ((char*) stack[sp].value) << ");" << endl;
							break;
						}
					}
					
					break;
				}
				
				case SWF_ACTION_CONSTANT_POOL:
				{
					action_buffer += length;
					
					break;
				}
				
				case SWF_ACTION_PUSH:
				{
					u64 push_value;
					size_t push_length = 0;
					
					while (push_length < length)
					{
						ActionStackValueType push_type = (ActionStackValueType) action_buffer[0];
						push_length += 1;
						action_buffer += 1;
						
						switch (push_type)
						{
							case ACTION_STACK_VALUE_STRING:
							{
								push_value = (u64) action_buffer;
								size_t push_str_len = strnlen((char*) push_value, 1024) + 1;
								push_length += push_str_len;
								
								if (push_length == 1024)
								{
									EXC("You can't be serious.\n");
								}
								
								action_buffer += push_str_len;
								
								break;
							}
							
							case ACTION_STACK_VALUE_F32:
							{
								push_value = (u64) VAL(u32, action_buffer);
								push_length += 4;
								action_buffer += 4;
								
								break;
							}
						}
						
						push(push_type, push_value);
					}
					
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
	
	void SWFAction::push(ActionStackValueType type, u64 value)
	{
		if (sp >= stack_size)
		{
			stack_size <<= 1;
			ActionStackValue* new_stack = new ActionStackValue[stack_size];
			
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