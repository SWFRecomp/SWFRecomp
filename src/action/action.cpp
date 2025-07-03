#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <action.hpp>

#define VAL(type, x) *((type*) x)

using std::stringstream;
using std::to_string;
using std::endl;

namespace SWFRecomp
{
	SWFAction::SWFAction() : next_str_i(0)
	{
		
	}
	
	char* SWFAction::parseActions(char* action_buffer, ofstream& out_script, ofstream& out_script_defs, ofstream& out_script_decls)
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
				
				case SWF_ACTION_STOP:
				{
					out_script << "\t" << "// Stop" << endl
							   << "\t" << "quit_swf = 1;" << endl;
					
					break;
				}
				
				case SWF_ACTION_ADD:
				{
					out_script << "\t" << "// Add" << endl
							   << "\t" << "actionAdd(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_SUBTRACT:
				{
					out_script << "\t" << "// Subtract" << endl
							   << "\t" << "actionSubtract(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_MULTIPLY:
				{
					out_script << "\t" << "// Multiply" << endl
							   << "\t" << "actionMultiply(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_DIVIDE:
				{
					out_script << "\t" << "// Divide" << endl
							   << "\t" << "actionDivide(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_EQUALS:
				{
					out_script << "\t" << "// Equals" << endl
							   << "\t" << "actionEquals(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_LESS:
				{
					out_script << "\t" << "// Less" << endl
							   << "\t" << "actionLess(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_AND:
				{
					out_script << "\t" << "// And" << endl
							   << "\t" << "actionAnd(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_OR:
				{
					out_script << "\t" << "// Or" << endl
							   << "\t" << "actionOr(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_NOT:
				{
					out_script << "\t" << "// Not" << endl
							   << "\t" << "actionNot(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_EQUALS:
				{
					declareEmptyString(17, out_script_defs, out_script_decls);
					declareEmptyString(17, out_script_defs, out_script_decls);
					
					out_script << "\t" << "// StringEquals" << endl
							   << "\t" << "actionStringEquals(stack, sp, "
							   << "str_" << to_string(next_str_i - 2) << ", "
							   << "str_" << to_string(next_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_LENGTH:
				{
					declareEmptyString(17, out_script_defs, out_script_decls);
					
					out_script << "\t" << "// StringLength" << endl
							   << "\t" << "actionStringLength(stack, sp, str_"
							   << to_string(next_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_ADD:
				{
					declareEmptyString(17, out_script_defs, out_script_decls);
					declareEmptyString(17, out_script_defs, out_script_decls);
					
					out_script << "\t" << "// StringAdd" << endl
							   << "\t" << "actionStringAdd(stack, sp, "
							   << "str_" << to_string(next_str_i - 2) << ", "
							   << "str_" << to_string(next_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_POP:
				{
					out_script << "\t" << "// Pop" << endl
							   << "\t" << "POP();" << endl;
					
					break;
				}
				
				case SWF_ACTION_GET_VARIABLE:
				{
					out_script << "\t" << "// GetVariable" << endl
							   << "\t" << "temp_val = getVariable((char*) STACK_TOP_VALUE, STACK_TOP_N);" << endl
							   << "\t" << "POP();" << endl
							   << "\t" << "PUSH_VAR(temp_val);" << endl;
					
					break;
				}
				
				case SWF_ACTION_SET_VARIABLE:
				{
					out_script << "\t" << "// SetVariable" << endl
							   << "\t" << "temp_val = getVariable((char*) STACK_SECOND_TOP_VALUE, STACK_SECOND_TOP_N);" << endl
							   << "\t" << "SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);" << endl
							   << "\t" << "POP_2();" << endl;
					
					break;
				}
				
				case SWF_ACTION_TRACE:
				{
					out_script << "\t" << "// Trace" << endl
							   << "\t" << "actionTrace(stack, sp);" << endl;
					
					break;
				}
				
				case SWF_ACTION_GET_TIME:
				{
					out_script << "\t" << "// GetTime" << endl
							   << "\t" << "actionGetTime(stack, sp);" << endl;
					
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
						ActionStackValueType push_type = (ActionStackValueType) action_buffer[push_length];
						push_length += 1;
						
						out_script << "\t" << "// Push ";
						
						switch (push_type)
						{
							case ACTION_STACK_VALUE_STRING:
							{
								out_script << "(String)" << endl;
								
								push_value = (u64) &action_buffer[push_length];
								declareString((char*) push_value, out_script_defs, out_script_decls);
								size_t push_str_len = strlen((char*) push_value);
								push_length += push_str_len + 1;
								
								out_script << "\t" << "PUSH_STR(str_" << to_string(next_str_i - 1) << ", " << push_str_len << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_F32:
							{
								out_script << "(float)" << endl;
								
								push_value = (u64) VAL(u32, &action_buffer[push_length]);
								push_length += 4;
								
								char hex_float[11];
								snprintf(hex_float, 11, "0x%08X", (u32) push_value);
								
								out_script << "\t" << "PUSH(ACTION_STACK_VALUE_F32, " << hex_float << ");" << endl;
								
								break;
							}
							
							default:
							{
								EXC_ARG("Undefined push type: %d\n", push_type);
							}
						}
					}
					
					action_buffer += push_length;
					
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
	
	void SWFAction::declareVariable(char* var_name, ostream& out_script_defs, ostream& out_script_decls)
	{
		out_script_defs << endl << "#ifndef DEF_VAR_" << var_name << endl
						<< "#define DEF_VAR_" << var_name << endl
						<< "var " << var_name << ";" << endl
						<< "#endif";
		
		out_script_decls << endl << "extern var " << var_name << ";";
	}
	
	void SWFAction::declareString(char* str, ostream& out_script_defs, ostream& out_script_decls)
	{
		out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
		out_script_decls << endl << "extern char* str_" << next_str_i << ";";
		next_str_i += 1;
	}
	
	void SWFAction::declareEmptyString(size_t size, ostream& out_script_defs, ostream& out_script_decls)
	{
		out_script_defs << endl << "char str_" << next_str_i << "[" << to_string(size) << "];";
		out_script_decls << endl << "extern char str_" << next_str_i << "[];";
		next_str_i += 1;
	}
	
	char SWFAction::actionCodeLookAhead(char* action_buffer, int lookAhead)
	{
		return action_buffer[actionCodeLookAheadIndex(action_buffer, lookAhead)];
	}
	
	size_t SWFAction::actionCodeLookAheadIndex(char* action_buffer, int lookAhead)
	{
		size_t action_buffer_i = 0;
		
		for (int i = 0; i < lookAhead; ++i)
		{
			if ((action_buffer[action_buffer_i] & 0b10000000) != 0)
			{
				action_buffer_i += 1;
				action_buffer_i += VAL(u16, &action_buffer[action_buffer_i]);
				action_buffer_i += 2;
			}
			
			else
			{
				action_buffer_i += 1;
			}
		}
		
		return action_buffer_i;
	}
};