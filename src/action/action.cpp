#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <tag.hpp>
#include <action.hpp>

#define VAL(type, x) *((type*) x)

using std::stringstream;
using std::to_string;
using std::endl;

namespace SWFRecomp
{
	SWFAction::SWFAction() : next_str_i(1), next_empty_str_i(0), func_counter(0)
	{
		
	}
	
	SWFAction::SWFAction(Context& context, const std::vector<std::string>& initial_strings) : next_str_i(1), next_empty_str_i(0), func_counter(0)
	{
		for (const std::string& s : initial_strings)
		{
			declareString(context, s.c_str());
		}
	}
	
	bool SWFAction::parseActions(Context& context, char*& action_buffer, ostream& out_script, bool should_emit_return, bool is_function, char* stop_at)
	{
		SWFActionType code = SWF_ACTION_CONSTANT_POOL;
		u16 length;
		bool last_action_return = false;
		
		char* action_buffer_start = action_buffer;
		
		std::vector<char*> labels;
		
		// Parse action bytes once to mark labels
		while (code != SWF_ACTION_END_OF_ACTIONS && action_buffer != stop_at)
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
				case SWF_ACTION_JUMP:
				case SWF_ACTION_IF:
				{
					s16 offset = VAL(s16, action_buffer);
					labels.push_back(action_buffer + length + ((s64) offset));
					break;
				}
			}
			
			action_buffer += length;
		}
		
		action_buffer = action_buffer_start;
		code = SWF_ACTION_CONSTANT_POOL;
		
		while (code != SWF_ACTION_END_OF_ACTIONS && action_buffer != stop_at)
		{
			for (const char* ptr : labels)
			{
				if (action_buffer == ptr)
				{
					out_script << "label_" << to_string((s16) (ptr - action_buffer_start)) << ":" << endl;
				}
			}
			
			code = (SWFActionType) (u8) action_buffer[0];
			
			last_action_return = code == SWF_ACTION_RETURN;
			
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
							   << "\t" << "actionAdd(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_SUBTRACT:
				{
					out_script << "\t" << "// Subtract" << endl
							   << "\t" << "actionSubtract(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_MULTIPLY:
				{
					out_script << "\t" << "// Multiply" << endl
							   << "\t" << "actionMultiply(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_DIVIDE:
				{
					out_script << "\t" << "// Divide" << endl
							   << "\t" << "actionDivide(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_EQUALS:
				{
					out_script << "\t" << "// Equals" << endl
							   << "\t" << "actionEquals(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_LESS:
				{
					out_script << "\t" << "// Less" << endl
							   << "\t" << "actionLess(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_AND:
				{
					out_script << "\t" << "// And" << endl
							   << "\t" << "actionAnd(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_OR:
				{
					out_script << "\t" << "// Or" << endl
							   << "\t" << "actionOr(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_NOT:
				{
					out_script << "\t" << "// Not" << endl
							   << "\t" << "actionNot(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_STRING_EQUALS:
				{
					declareEmptyString(context, 17);
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// StringEquals" << endl
							   << "\t" << "actionStringEquals(app_context, "
							   << "str_" << to_string(next_empty_str_i - 2) << ", "
							   << "str_" << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_LENGTH:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// StringLength" << endl
							   << "\t" << "actionStringLength(app_context, str_"
							   << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_STRING_ADD:
				{
					declareEmptyString(context, 17);
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// StringAdd" << endl
							   << "\t" << "actionStringAdd(app_context, "
							   << "str_" << to_string(next_empty_str_i - 2) << ", "
							   << "str_" << to_string(next_empty_str_i - 1) << ");" << endl;
					
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
							   << "\t" << "actionGetVariable(app_context);" << endl;
					break;
				}
				
				case SWF_ACTION_SET_VARIABLE:
				{
					out_script << "\t" << "// SetVariable" << endl
							   << "\t" << "actionSetVariable(app_context);" << endl;
					break;
				}
				
				case SWF_ACTION_TRACE:
				{
					out_script << "\t" << "// Trace" << endl
							   << "\t" << "actionTrace(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_GET_TIME:
				{
					out_script << "\t" << "// GetTime" << endl
							   << "\t" << "actionGetTime(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_CONSTANT_POOL:
				{
					u16 str_count = VAL(u16, action_buffer);
					action_buffer += 2;
					
					SWFTag constant_pool_tag;
					constant_pool_tag.clearFields();
					constant_pool_tag.setFieldCount(str_count);
					
					for (u16 i = 0; i < str_count; ++i)
					{
						constant_pool_tag.configureNextField(SWF_FIELD_STRING);
					}
					
					constant_pool_tag.parseFields(action_buffer);
					
					constant_pool.clear();
					
					Constant c;
					
					for (u16 i = 0; i < str_count; ++i)
					{
						c.str_id = getStringId(context, (char*) constant_pool_tag.fields[i].value);
						c.str_length = (size_t) constant_pool_tag.fields[i].str_length;
						constant_pool.push_back(c);
					}
					
					break;
				}
				
				case SWF_ACTION_PUSH:
				{
					u64 push_value;
					size_t push_length = 0;
					
					while (push_length < length)
					{
						ActionStackValueType push_type = (ActionStackValueType) action_buffer[0];
						action_buffer += 1;
						push_length += 1;
						
						out_script << "\t" << "// Push ";
						
						switch (push_type)
						{
							case ACTION_STACK_VALUE_STRING:
							{
								out_script << "(String)" << endl;
								
								SWFTag tag = SWFTag();
								tag.clearFields();
								tag.setFieldCount(1);
								tag.configureNextField(SWF_FIELD_STRING);
								tag.parseFields(action_buffer);
								
								push_value = tag.fields[0].value;
								size_t push_str_len = tag.fields[0].str_length;
								push_length += push_str_len + 1;
								
								// Get the actual string ID (handles deduplication)
								size_t str_id = getStringId(context, (char*) push_value);
								
								out_script << "\t" << "PUSH_STR_ID(const_str_" << to_string(str_id) << ", "
								           << str_id << ", " << push_str_len << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_F32:
							{
								out_script << "(f32)" << endl;
								
								push_value = (u64) VAL(u32, action_buffer);
								action_buffer += 4;
								push_length += 4;
								
								char hex_float[11];
								snprintf(hex_float, 11, "0x%08X", (u32) push_value);
								
								out_script << "\t" << "PUSH(ACTION_STACK_VALUE_F32, " << hex_float << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_REGISTER:
							{
								out_script << "(register)" << endl;
								
								u8 reg = VAL(u8, action_buffer);
								action_buffer += 1;
								push_length += 1;
								
								out_script << "\t" << "PUSH(ACTION_STACK_VALUE_REGISTER, " << to_string(reg) << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_F64:
							{
								out_script << "(f64)" << endl;
								
								push_value = VAL(u32, action_buffer);
								action_buffer += 4;
								push_length += 4;
								
								push_value <<= 32;
								
								push_value |= VAL(u32, action_buffer);
								action_buffer += 4;
								push_length += 4;
								
								char hex_float[19];
								snprintf(hex_float, 19, "0x%016llX", (u64) push_value);
								
								out_script << "\t" << "PUSH(ACTION_STACK_VALUE_F64, " << hex_float << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_NULL:
							{
								out_script << "(null)" << endl;
								
								out_script << "\t" << "PUSH_NULL();" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_UNDEFINED:
							{
								out_script << "(undefined)" << endl;
								
								out_script << "\t" << "PUSH_UNDEFINED();" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_INT:
							{
								out_script << "(integer)" << endl;
								
								push_value = (u64) VAL(u32, action_buffer);
								action_buffer += 4;
								push_length += 4;
								
								out_script << "\t" << "PUSH(ACTION_STACK_VALUE_INT, " << to_string(push_value) << ");" << endl;
								
								break;
							}
							
							case ACTION_STACK_VALUE_CONST8:
							{
								out_script << "(String)" << endl;
								
								u8 const_index = VAL(u8, action_buffer);
								action_buffer += 1;
								push_length += 1;
								
								Constant& c = constant_pool[const_index];
								
								out_script << "\t" << "PUSH_STR_ID(const_str_" << to_string(c.str_id) << ", "
								           << c.str_id << ", " << c.str_length << ");" << endl;
								
								break;
							}
							
							default:
							{
								EXC_ARG("Undefined push type: %d\n", push_type);
							}
						}
					}
					
					break;
				}
				
				case SWF_ACTION_JUMP:
				{
					s16 offset = VAL(s16, action_buffer);
					
					out_script << "\t" << "// Jump" << endl
							   << "\t" << "goto label_" << to_string((s16) (action_buffer + length - action_buffer_start + offset)) << ";" << endl;
							
					action_buffer += length;
					
					break;
				}
				
				case SWF_ACTION_IF:
				{
					s16 offset = VAL(s16, action_buffer);
					
					out_script << "\t" << "// If" << endl
							   << "\t" << "if (evaluateCondition(app_context))" << endl
							   << "\t" << "{" << endl
							   << "\t" << "\t" << "goto label_" << to_string((s16) (action_buffer + length - action_buffer_start + offset)) << ";" << endl
							   << "\t" << "}" << endl;
							
					action_buffer += length;
					
					break;
				}
				
				case SWF_ACTION_DELETE:
				{
					out_script << "\t" << "// Delete" << endl
							   << "\t" << "actionDelete(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_DELETE2:
				{
					declareEmptyString(context, 256);  // Buffer for variable name operations
					
					out_script << "\t" << "// Delete2" << endl
							   << "\t" << "actionDelete2(app_context, "
							   << "str_" << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_DEFINE_LOCAL:
				{
					out_script << "\t" << "// DefineLocal" << endl
							   << "\t" << "actionDefineLocal(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_CALL_FUNCTION:
				{
					out_script << "\t" << "// CallFunction" << endl
							   << "\t" << "actionCallFunction(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_RETURN:
				{
					out_script << "\t" << "// Return" << endl
							   << "\t" << "return;" << endl;
					
					break;
				}
				
				case SWF_ACTION_NEW_OBJECT:
				{
					out_script << "\t" << "// NewObject" << endl
							   << "\t" << "actionNewObject(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_DECLARE_LOCAL:
				{
					out_script << "\t" << "// DefineLocal2" << endl
							   << "\t" << "actionDefineLocal2(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_INIT_ARRAY:
				{
					out_script << "\t" << "// InitArray" << endl
							   << "\t" << "actionInitArray(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_INIT_OBJECT:
				{
					out_script << "\t" << "// InitObject" << endl
							   << "\t" << "actionInitObject(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_TYPEOF:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// Typeof" << endl
							   << "\t" << "actionTypeof(app_context, str_"
							   << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_ENUMERATE:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// Enumerate" << endl
							   << "\t" << "actionEnumerate(app_context, str_"
							   << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_GET_MEMBER:
				{
					out_script << "\t" << "// GetMember" << endl
							   << "\t" << "actionGetMember(app_context);" << endl;
					
					break;
				}
				
				case SWF_ACTION_SET_MEMBER:
				{
					out_script << "\t" << "// SetMember" << endl
							   << "\t" << "actionSetMember(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_CALL_METHOD:
				{
					out_script << "\t" << "// CallMethod" << endl
							   << "\t" << "actionCallMethod(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_NEW_METHOD:
				{
					out_script << "\t" << "// NewMethod" << endl
							   << "\t" << "actionNewMethod(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_INSTANCEOF:
				{
					out_script << "\t" << "// InstanceOf" << endl
							   << "\t" << "actionInstanceOf(app_context);" << endl;
							
					break;
				}
				
				case SWF_ACTION_ENUMERATE2:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// Enumerate2" << endl
							   << "\t" << "actionEnumerate2(app_context, str_"
							   << to_string(next_empty_str_i - 1) << ");" << endl;
					
					break;
				}
				
				case SWF_ACTION_EXTENDS:
				{
					out_script << "\t" << "// Extends - Set up prototype chain for inheritance" << endl
							   << "\t" << "actionExtends(app_context);" << endl;
					
					break;
				}
				
				case SWF_ACTION_STORE_REGISTER:
				{
					// Read register number from bytecode
					u8 register_num = (u8) action_buffer[0];
					
					out_script << "\t" << "// StoreRegister " << (int)register_num << endl;
					out_script << "\t" << "actionStoreRegister(app_context, " << (int)register_num << ");" << endl;
					
					action_buffer += length;
					break;
				}
				
				case SWF_ACTION_DEFINE_FUNCTION2:
				{
					// Parse function metadata
					char* func_name = action_buffer;
					size_t name_len = strlen(func_name);
					action_buffer += name_len + 1;
					
					u16 num_params = VAL(u16, action_buffer);
					action_buffer += 2;
					
					u8 reg_count = VAL(u8, action_buffer);
					action_buffer += 1;
					
					u16 flags = VAL(u16, action_buffer);
					action_buffer += 2;
					
					func_id_to_type[func_counter] = FUNC_TYPE_2;
					func_id_to_param_reg_counts[func_counter] = reg_count;
					func_id_to_param_flags[func_counter] = flags;
					
					// Parse parameter names and registers
					func_id_to_param_string_ids[func_counter] = std::vector<size_t>();
					func_id_to_param_regs[func_counter] = std::vector<Function2Param>();
					std::vector<size_t>& params = func_id_to_param_string_ids[func_counter];
					std::vector<Function2Param>& param_regs = func_id_to_param_regs[func_counter];
					
					SWFTag tag = SWFTag();
					
					tag.clearFields();
					tag.setFieldCount(2);
					tag.configureNextField(SWF_FIELD_UI8);
					tag.configureNextField(SWF_FIELD_STRING);
					
					for (u16 i = 0; i < num_params; i++)
					{
						tag.parseFields(action_buffer);
						u8 reg = (u8) tag.fields[0].value;
						char* param_name = (char*) tag.fields[1].value;
						
						size_t string_id = getStringId(context, param_name);
						
						if (reg == 0)
						{
							params.push_back(string_id);
						}
						
						else
						{
							Function2Param p;
							p.reg = reg;
							p.string_id = string_id;
							param_regs.push_back(p);
						}
					}
					
					u16 code_size = VAL(u16, action_buffer);
					action_buffer += 2;
					
					bool anonymous = name_len == 0;
					
					// Generate unique function ID
					std::string func_id = (!anonymous ? std::string(func_name) : std::string("anonymous")) + "_" + std::to_string(func_counter);
					
					size_t string_id = getStringId(context, func_name);
					func_id_to_stream[func_counter] = std::stringstream();
					ostream& func_stream = func_id_to_stream[func_counter];
					
					// Add function declaration to header (uses app_context)
					context.out_script_decls << endl << "void " << func_id << "(SWFAppContext* app_context);" << endl;
					
					// Generate function definition
					func_stream << "// DefineFunction2: " << (!anonymous ? func_name : "(anonymous)") << endl
								<< "void " << func_id << "(SWFAppContext* app_context)" << endl
								<< "{" << endl;
					
					// Parse function body recursively
					func_stream << "\t// Function body (" << code_size << " bytes)" << endl << "\t" << endl;
					
					bool has_return = parseActions(context, action_buffer, func_stream, true, true, action_buffer + code_size);
					
					func_stream << "}";
					
					// Generate runtime call to register function
					out_script << "\t// DefineFunction2" << endl;
					out_script << "\tactionDefineFunction2(app_context, "
							   << to_string(string_id) << ", "
							   << func_id << ", "
							   << "func_params_" << func_counter << ", "
							   << "func_params_" << func_counter << "_reg_count, "
							   << "func_params_" << func_counter << "_flags, "
							   << (anonymous ? "true" : "false") << ");" << endl;
					
					func_counter += 1;
					break;
				}
				
				case SWF_ACTION_DEFINE_FUNCTION:
				{
					// Parse function metadata
					char* func_name = action_buffer;
					size_t name_len = strlen(func_name);
					action_buffer += name_len + 1;
					
					u16 num_params = VAL(u16, action_buffer);
					action_buffer += 2;
					
					func_id_to_type[func_counter] = FUNC_TYPE_1;
					
					// Parse parameter names
					func_id_to_param_string_ids[func_counter] = std::vector<size_t>();
					std::vector<size_t>& params = func_id_to_param_string_ids[func_counter];
					for (u16 i = 0; i < num_params; i++)
					{
						char* param_name = action_buffer;
						action_buffer += strlen(param_name) + 1;
						params.push_back(getStringId(context, param_name));
					}
					
					u16 code_size = VAL(u16, action_buffer);
					action_buffer += 2;
					
					bool anonymous = name_len == 0;
					
					// Generate unique function ID
					std::string func_id = (!anonymous ? std::string(func_name) : std::string("anonymous")) + "_" + std::to_string(func_counter);
					
					size_t string_id = getStringId(context, func_name);
					func_id_to_stream[func_counter] = std::stringstream();
					ostream& func_stream = func_id_to_stream[func_counter];
					
					// Add function declaration to header (uses app_context)
					context.out_script_decls << endl << "void " << func_id << "(SWFAppContext* app_context);" << endl;
					
					// Generate function definition
					func_stream << "// DefineFunction: " << (!anonymous ? func_name : "(anonymous)") << endl
								<< "void " << func_id << "(SWFAppContext* app_context)" << endl
								<< "{" << endl;
					
					// Parse function body recursively
					func_stream << "\t// Function body (" << code_size << " bytes)" << endl << "\t" << endl;
					
					bool has_return = parseActions(context, action_buffer, func_stream, true, true, action_buffer + code_size);
					
					func_stream << "}";
					
					// Generate runtime call to register function
					out_script << "\t// DefineFunction" << endl;
					out_script << "\tactionDefineFunction(app_context, "
							   << to_string(string_id) << ", "
							   << func_id << ", "
							   << "func_params_" << func_counter << ", "
							   << (anonymous ? "true" : "false") << ");" << endl;
					
					func_counter += 1;
					
					break;
				}
				
				default:
				{
					EXC_ARG("Unimplemented action 0x%02X\n", code);
					
					break;
				}
			}
		}
		
		if (should_emit_return)
		{
			if (is_function && !last_action_return)
			{
				out_script << "\t// Return (void)" << endl
						   << "\tPUSH_UNDEFINED();" << endl;
			}
			
			out_script << "\treturn;" << endl;
		}
		
		return last_action_return;
	}
	
	void SWFAction::recompileStringTable(Context& context)
	{
		context.out_script_decls << endl << "char* str_table["
								 << next_str_i << "];";
		
		context.out_script_defs << endl << "char* str_table["
								<< next_str_i << "] ="
								<< endl << "{"
								<< endl << "\tNULL," << endl;
		
		for (size_t i = 1; i < next_str_i; ++i)
		{
			context.out_script_defs << "\t\"" << id_to_string[i].c_str() << "\"," << endl;
		}
		
		context.out_script_defs << "};";
	}
	
	void SWFAction::recompileFunctionTable(Context& context)
	{
		size_t last_id = 0;
		size_t num_funcs_in_file = 0;
		
		for (size_t func_id = 0; func_id < func_counter; ++func_id)
		{
			switch (func_id_to_type[func_id])
			{
				case FUNC_TYPE_1:
				{
					std::vector<size_t>& params = func_id_to_param_string_ids[func_id];
					
					if (params.size() == 0)
					{
						context.out_script_decls << endl << "extern u32* func_params_" << func_id << ";";
						
						context.out_script_defs << endl << "u32* func_params_" << func_id
												<< " = ";
						
						context.out_script_defs << "NULL;";
					}
					
					else
					{
						context.out_script_decls << endl << "extern u32 func_params_" << func_id << "[" << params.size() << "];";
						context.out_script_defs << endl << "u32 func_params_" << func_id
												<< "[] = ";
						
						context.out_script_defs << "{ ";
						
						for (size_t i = 0; i < params.size(); ++i)
						{
							context.out_script_defs << params[i] << ", ";
						}
						
						context.out_script_defs << "};";
					}
					
					break;
				}
				
				case FUNC_TYPE_2:
				{
					std::vector<Function2Param>& params = func_id_to_param_regs[func_id];
					
					if (params.size() == 0)
					{
						context.out_script_decls << endl << "extern Function2Param* func_params_" << func_id << ";";
						
						context.out_script_defs << endl << "Function2Param* func_params_" << func_id
												<< " = ";
						
						context.out_script_defs << "NULL;";
					}
					
					else
					{
						context.out_script_decls << endl << "extern Function2Param func_params_" << func_id << "[" << params.size() << "];";
						context.out_script_defs << endl << "Function2Param func_params_" << func_id
												<< "[] = ";
						
						context.out_script_defs << "{ ";
						
						for (size_t i = 0; i < params.size(); ++i)
						{
							context.out_script_defs << "{ ";
							context.out_script_defs << to_string(params[i].reg) << ", ";
							context.out_script_defs << to_string(params[i].string_id) << " }, ";
						}
						
						context.out_script_defs << "};";
					}
					
					context.out_script_decls << endl << "extern u8 func_params_" << func_id << "_reg_count;";
					context.out_script_defs << endl << "u8 func_params_" << func_id
											<< "_reg_count = " << to_string(func_id_to_param_reg_counts[func_id]) << ";";
					
					context.out_script_decls << endl << "extern u16 func_params_" << func_id << "_flags;";
					context.out_script_defs << endl << "u16 func_params_" << func_id
											<< "_flags = " << to_string(func_id_to_param_flags[func_id]) << ";";
					
					break;
				}
			}
			
			if (context.num_files == 0 || num_funcs_in_file >= context.config.funcs_per_file)
			{
				context.out_funcs.push_back(ofstream(context.config.output_scripts_folder + "funcs_" + to_string(context.num_files) + ".c", std::ios_base::out));
				context.num_files += 1;
				num_funcs_in_file = 0;
				
				context.out_funcs.back() << "#include <recomp.h>" << endl << "#include \"script_decls.h\"";
			}
			
			context.out_funcs.back() << endl << endl << func_id_to_stream[func_id].str();
			
			num_funcs_in_file += 1;
		}
	}
	
	void SWFAction::declareVariable(Context& context, char* var_name)
	{
		context.out_script_defs << endl << "#ifndef DEF_VAR_" << var_name << endl
								<< "#define DEF_VAR_" << var_name << endl
								<< "var " << var_name << ";" << endl
								<< "#endif";
								
		context.out_script_decls << endl << "extern var " << var_name << ";";
	}
	
	void SWFAction::declareString(Context& context, const char* str)
	{
		// New string - assign ID and declare
		string_to_id[str] = next_str_i;
		id_to_string[next_str_i] = str;
		context.out_script_defs << endl << "char* const_str_" << next_str_i << " = \"" << str << "\";";
		context.out_script_decls << endl << "extern char* const_str_" << next_str_i << ";";
		next_str_i += 1;
	}
	
	void SWFAction::declareEmptyString(Context& context, size_t size)
	{
		context.out_script_defs << endl << "char str_" << next_empty_str_i << "[" << to_string(size) << "];";
		context.out_script_decls << endl << "extern char str_" << next_empty_str_i << "[];";
		next_empty_str_i += 1;
	}
	
	size_t SWFAction::getStringId(Context& context, const char* str)
	{
		auto it = string_to_id.find(str);
		if (it != string_to_id.end())
		{
			return it->second;
		}
		
		size_t id = next_str_i;
		declareString(context, str);
		return id;
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