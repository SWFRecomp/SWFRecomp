#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <action.hpp>

#define VAL(type, x) *((type*) x)
#define VD_STR ((char*) vardetect_value)

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
		std::streampos vardetect_backtrack = 0;
		std::streampos vardetect_backtrack_prev = 0;
		u64 vardetect_value = 0;
		u64 dynavar_value = 0;
		bool setvardetect_possible = false;
		
		ActionStackValue last_push;
		last_push.type = ACTION_STACK_VALUE_UNSET;
		last_push.value = ACTION_STACK_VALUE_UNSET;
		
		ActionStackValue second_last_push;
		
		stringstream pushes;
		
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
			
			if (setvardetect_possible && code != SWF_ACTION_PUSH)
			{
				out_script << pushes.str().substr(0, pushes.tellp()).c_str();
				pushes.str("");
				pushes.clear();
				
				vardetect_value = 0;
				second_last_push.type = ACTION_STACK_VALUE_UNSET;
				last_push.type = ACTION_STACK_VALUE_UNSET;
				dynavar_value = 0;
				setvardetect_possible = false;
			}
			
			switch (code)
			{
				case SWF_ACTION_END_OF_ACTIONS:
				{
					break;
				}
				
				case SWF_ACTION_ADD:
				{
					out_script << "\t" << "// Add" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionAdd(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_SUBTRACT:
				{
					out_script << "\t" << "// Subtract" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionSubtract(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_MULTIPLY:
				{
					out_script << "\t" << "// Multiply" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionMultiply(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_DIVIDE:
				{
					out_script << "\t" << "// Divide" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionDivide(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_EQUALS:
				{
					out_script << "\t" << "// Equals" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionEquals(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_LESS:
				{
					out_script << "\t" << "// Less" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionLess(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_AND:
				{
					out_script << "\t" << "// And" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionAnd(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_OR:
				{
					out_script << "\t" << "// Or" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "actionOr(&stack[sp], &stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_NOT:
				{
					out_script << "\t" << "// Not" << endl
							   << "\t" << "actionNot(&stack[sp - 1]);" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_EQUALS:
				{
					out_script << "\t" << "// StringEquals" << endl
							   << "\t" << "sp -= 1;" << endl
							   << "\t" << "char str_" << to_string(next_str_i) << "[17];" << endl
							   << "\t" << "char str_" << to_string(next_str_i + 1) << "[17];" << endl
							   << "\t" << "actionStringEquals(&stack[sp], &stack[sp - 1], "
							   << "str_" << to_string(next_str_i) << ", str_" << to_string(next_str_i + 1) << ");" << endl;
					
					next_str_i += 2;
					
					break;
				}
				
				case SWF_ACTION_POP:
				{
					out_script << "\t" << "// Pop" << endl
							   << "\t" << "sp -= 1;" << endl;
					
					break;
				}
				
				case SWF_ACTION_GET_VARIABLE:
				{
					out_script << "\t" << "// GetVariable";
					
					if (vardetect_value != 0)
					{
						out_script << " (static var holds dynamic name)" << endl
								   << "\t" << "DECL_TEMP_VAR_PTR temp_val = getVariable(" << VD_STR << ".value);" << endl
								   << "\t" << "#undef DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "#define DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "stack[sp].type = temp_val->type;" << endl
								   << "\t" << "stack[sp].value = temp_val->value;" << endl
								   << "\t" << "sp += 1;" << endl;
						vardetect_value = 0;
						pushes.str("");
						pushes.clear();
					}
					
					else
					{
						out_script << endl
								   << "\t" << "DECL_TEMP_VAR_PTR temp_val = getVariable(stack[sp - 1].value);" << endl
								   << "\t" << "#undef DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "#define DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "stack[sp - 1].type = temp_val->type;" << endl
								   << "\t" << "stack[sp - 1].value = temp_val->value;" << endl;
					}
					
					break;
				}
				
				case SWF_ACTION_SET_VARIABLE:
				{
					out_script << "\t" << "// SetVariable";
					
					if (vardetect_value != 0)
					{
						out_script << " (static var holds dynamic name)" << endl
								   << "\t" << "sp -= 2;" << endl;
						
						declareVariable(VD_STR, out_script_defs, out_script_decls);
						
						out_script << "\t" << "DECL_TEMP_VAR_PTR temp_val = getVariable(" << VD_STR << ".value);" << endl;
						vardetect_value = 0;
						pushes.str("");
						pushes.clear();
					}
					
					else
					{
						out_script << endl
								   << "\t" << "sp -= 2;" << endl
								   << "\t" << "DECL_TEMP_VAR_PTR temp_val = getVariable(stack[sp].value);" << endl;
					}
					
					out_script << "\t" << "#undef DECL_TEMP_VAR_PTR" << endl
							   << "\t" << "#define DECL_TEMP_VAR_PTR" << endl
							   << "\t" << "temp_val->type = stack[sp + 1].type;" << endl
							   << "\t" << "temp_val->value = stack[sp + 1].value;" << endl;
					
					break;
				}
				
				case SWF_ACTION_TRACE:
				{
					out_script << "\t" << "// Trace" << endl;
					
					if (vardetect_value != 0)
					{
						out_script << "\t" << "actionTrace(&" << VD_STR << ");" << endl;
						vardetect_value = 0;
						pushes.str("");
						pushes.clear();
					}
					
					else
					{
						out_script << "\t" << "actionTrace(&stack[--sp]);" << endl;
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
						ActionStackValueType push_type = (ActionStackValueType) action_buffer[push_length];
						push_length += 1;
						
						vardetect_backtrack_prev = vardetect_backtrack;
						vardetect_backtrack = pushes.tellp();
						
						pushes << "\t" << "// Push ";
						
						switch (push_type)
						{
							case ACTION_STACK_VALUE_STRING:
							{
								pushes << "(String)" << endl;
								
								push_value = (u64) &action_buffer[push_length];
								declareString((char*) push_value, out_script_defs, out_script_decls);
								size_t push_str_len = strnlen((char*) push_value, 1024) + 1;
								push_length += push_str_len;
								
								pushes << "\t" << "stack[sp].type = ACTION_STACK_VALUE_STRING;" << endl
									   << "\t" << "stack[sp].value = (u64) str_" << to_string(next_str_i - 1) << ";" << endl
									   << "\t" << "sp += 1;" << endl;
								
								if (push_length == 1024)
								{
									EXC("You can't be serious.\n");
								}
								
								break;
							}
							
							case ACTION_STACK_VALUE_F32:
							{
								pushes << "(float)" << endl;
								
								push_value = (u64) VAL(u32, &action_buffer[push_length]);
								push_length += 4;
								
								pushes << "\t" << "stack[sp].type = ACTION_STACK_VALUE_F32;" << endl
									   << "\t" << "stack[sp].value = " << ((u32) push_value) << ";" << endl
									   << "\t" << "sp += 1;" << endl;
								
								break;
							}
						}
						
						second_last_push.type = last_push.type;
						second_last_push.value = last_push.value;
						
						last_push.type = push_type;
						last_push.value = push_value;
					}
					
					action_buffer += push_length;
					
					//~ printf("status...last push type: %d, aCLA0: 0x%02X, aCLA1: 0x%02X, aCLA2: 0x%02X\n", last_push.type, (u8) actionCodeLookAhead(action_buffer, 0), (u8) actionCodeLookAhead(action_buffer, 1), (u8) actionCodeLookAhead(action_buffer, 2));
					
					if (dynavar_value != 0)
					{
						out_script << pushes.str().substr(0, pushes.tellp()).c_str();
						pushes.str("");
						pushes.clear();
						
						out_script << "\t" << "// SetVariable (dynamic consecutive get/set)" << endl
								   << "\t" << "sp -= 1;" << endl
								   << "\t" << "DECL_TEMP_VAR_PTR temp_val = getVariable(" << ((char*) dynavar_value) << ".value);" << endl
								   << "\t" << "#undef DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "#define DECL_TEMP_VAR_PTR" << endl
								   << "\t" << "temp_val->type = stack[sp].type;" << endl
								   << "\t" << "temp_val->value = stack[sp].value;" << endl;
						
						action_buffer += actionCodeLookAheadIndex(action_buffer, 1);
						
						dynavar_value = 0;
					}
					
					else if (last_push.type == ACTION_STACK_VALUE_STRING && (u8) actionCodeLookAhead(action_buffer, 0) == SWF_ACTION_GET_VARIABLE &&
							 (u8) actionCodeLookAhead(action_buffer, 1) == SWF_ACTION_PUSH && (u8) actionCodeLookAhead(action_buffer, 2) == SWF_ACTION_SET_VARIABLE)
					{
						// SetVariable using known-variable name's string detected
						action_buffer += actionCodeLookAheadIndex(action_buffer, 1);
						
						pushes.str("");
						pushes.clear();
						
						dynavar_value = last_push.value;
					}
					
					else if (last_push.type == ACTION_STACK_VALUE_STRING && action_buffer[0] == (u8) SWF_ACTION_GET_VARIABLE)
					{
						// GetVariable with known variable-name detected
						vardetect_value = last_push.value;
						second_last_push.type = ACTION_STACK_VALUE_UNSET;
						last_push.type = ACTION_STACK_VALUE_UNSET;
						setvardetect_possible = false;
						
						pushes.str("");
						pushes.clear();
						
						action_buffer += 1;
					}
					
					else if ((second_last_push.type == ACTION_STACK_VALUE_STRING && action_buffer[0] == (u8) SWF_ACTION_SET_VARIABLE) ||
							 (setvardetect_possible && action_buffer[0] == (u8) SWF_ACTION_SET_VARIABLE))
					{
						// SetVariable with known variable-name detected
						out_script << "\t" << "// SetVariable (static var)" << endl;
						
						pushes.seekp(vardetect_backtrack_prev, std::ios_base::beg);
						
						// Set this so VD_STR works
						vardetect_value = second_last_push.value;
						
						declareVariable(VD_STR, out_script_defs, out_script_decls);
						
						switch (last_push.type)
						{
							case ACTION_STACK_VALUE_STRING:
							{
								next_str_i -= 1;
								declareString((char*) last_push.value, out_script_defs, out_script_decls);
								pushes << "\t" << VD_STR << ".type = ACTION_STACK_VALUE_STRING" << ";" << endl
									   << "\t" << VD_STR << ".value = (u64) str_" << to_string(next_str_i - 1) << ";" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_F32:
							{
								pushes << "\t" << VD_STR << ".type = ACTION_STACK_VALUE_F32;" << endl
									   << "\t" << VD_STR << ".value = " << ((u32) push_value) << ";" << endl;
								
								break;
							}
						}
						
						out_script << pushes.str().substr(0, pushes.tellp()).c_str();
						pushes.str("");
						pushes.clear();
						
						vardetect_value = 0;
						second_last_push.type = ACTION_STACK_VALUE_UNSET;
						setvardetect_possible = false;
						
						action_buffer += 1;
					}
					
					else if (last_push.type == ACTION_STACK_VALUE_STRING)
					{
						// Optimized SetVariable is still possible
						// if another consecutive Push and then SetVariable
						setvardetect_possible = true;
					}
					
					else
					{
						out_script << pushes.str().substr(0, pushes.tellp()).c_str();
						pushes.str("");
						pushes.clear();
					}
					
					break;
				}
				
				default:
				{
					EXC_ARG("Unimplemented action 0x%02X\n", code);
					
					break;
				}
			}
			
			if (vardetect_value != 0 && code != SWF_ACTION_PUSH)
			{
				out_script << pushes.str().substr(0, pushes.tellp()).c_str();
				pushes.str("");
				pushes.clear();
				
				vardetect_value = 0;
				second_last_push.type = ACTION_STACK_VALUE_UNSET;
				last_push.type = ACTION_STACK_VALUE_UNSET;
				setvardetect_possible = false;
			}
		}
		
		return action_buffer;
	}
	
	void SWFAction::declareVariable(char* var_name, ostream& out_script_defs, ostream& out_script_decls)
	{
		out_script_defs << "#ifndef DECL_VAR_" << var_name << endl
						<< "#define DECL_VAR_" << var_name << endl
						<< "var " << var_name << ";" << endl
						<< "#endif" << endl;
		
		out_script_decls << "#ifndef DECL_VAR_" << var_name << endl
						 << "#define DECL_VAR_" << var_name << endl
						 << "extern var " << var_name << ";" << endl
						 << "#endif" << endl;
	}
	
	void SWFAction::declareString(char* str, ostream& out_script_defs, ostream& out_script_decls)
	{
		out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
		out_script_decls << endl << "extern char* str_" << next_str_i << ";";
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