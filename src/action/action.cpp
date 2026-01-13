#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <action.hpp>

#define VAL(type, x) *((type*) x)

using std::stringstream;
using std::to_string;
using std::endl;

namespace SWFRecomp
{
	SWFAction::SWFAction() : next_str_i(1), func_counter(0)
	{
	
	}
	
	void SWFAction::parseActions(Context& context, char*& action_buffer, ofstream& out_script)
	{
		SWFActionType code = SWF_ACTION_CONSTANT_POOL;
		u16 length;
		
		char* action_buffer_start = action_buffer;
		
		std::vector<char*> labels;
		
		// Parse action bytes once to mark labels
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
		
		while (code != SWF_ACTION_END_OF_ACTIONS)
		{
			for (const char* ptr : labels)
			{
				if (action_buffer == ptr)
				{
					out_script << "label_" << to_string((s16) (ptr - action_buffer_start)) << ":" << endl;
				}
			}
			
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
							   << "str_" << to_string(next_str_i - 2) << ", "
							   << "str_" << to_string(next_str_i - 1) << ");" << endl;
							
					break;
				}
				
				case SWF_ACTION_STRING_LENGTH:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// StringLength" << endl
							   << "\t" << "actionStringLength(app_context, str_"
							   << to_string(next_str_i - 1) << ");" << endl;
							
					break;
				}
				
				case SWF_ACTION_STRING_ADD:
				{
					declareEmptyString(context, 17);
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// StringAdd" << endl
							   << "\t" << "actionStringAdd(app_context, "
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
								declareString(context, (char*) push_value);
								size_t push_str_len = strlen((char*) push_value);
								push_length += push_str_len + 1;
								
								// Get the actual string ID (handles deduplication)
								size_t str_id = getStringId((char*) push_value);
								
								out_script << "\t" << "PUSH_STR_ID(str_" << to_string(str_id) << ", "
								           << push_str_len << ", " << str_id << ");" << endl;
								
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
							   << "str_" << to_string(next_str_i - 1) << ");" << endl;
							
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
							   << "\t" << "actionCallFunction(app_context, str_buffer);" << endl;
							
					break;
				}
				
				case SWF_ACTION_RETURN:
				{
					out_script << "\t" << "// Return" << endl
							   << "\t" << "{" << endl
							   << "\t\t" << "ActionVar ret_val;" << endl
							   << "\t\t" << "popVar(app_context, &ret_val);" << endl
							   << "\t\t" << "return ret_val;" << endl
							   << "\t" << "}" << endl;
							
					break;
				}
				
				// REMOVED OPCODE: Modulo - not supported in minimal build
				case SWF_ACTION_MODULO:
				{
					EXC_ARG("Opcode 0x%02X not supported in minimal build (objects/functions only)\n", code);
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
					out_script << "\t" << "// DeclareLocal" << endl
							   << "\t" << "actionDeclareLocal(app_context);" << endl;
							
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
							   << to_string(next_str_i - 1) << ");" << endl;
							
					break;
				}
				
				case SWF_ACTION_ENUMERATE:
				{
					declareEmptyString(context, 17);
					
					out_script << "\t" << "// Enumerate" << endl
							   << "\t" << "actionEnumerate(app_context, str_"
							   << to_string(next_str_i - 1) << ");" << endl;
							
					break;
				}
				
				// REMOVED OPCODE: Add2 - not supported in minimal build
				case SWF_ACTION_ADD2:
				// REMOVED OPCODE: Less2 - not supported in minimal build
				case SWF_ACTION_LESS2:
				// REMOVED OPCODE: Equals2 - not supported in minimal build
				case SWF_ACTION_EQUALS2:
				// REMOVED OPCODE: ToNumber - not supported in minimal build
				case SWF_ACTION_TO_NUMBER:
				// REMOVED OPCODE: ToString - not supported in minimal build
				case SWF_ACTION_TO_STRING:
				// REMOVED OPCODE: Duplicate - not supported in minimal build
				case SWF_ACTION_DUPLICATE:
				// REMOVED OPCODE: StackSwap - not supported in minimal build
				case SWF_ACTION_STACK_SWAP:
				{
					EXC_ARG("Opcode 0x%02X not supported in minimal build (objects/functions only)\n", code);
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
				
				// REMOVED OPCODE: Increment - not supported in minimal build
				case SWF_ACTION_INCREMENT:
				// REMOVED OPCODE: Decrement - not supported in minimal build
				case SWF_ACTION_DECREMENT:
				{
					EXC_ARG("Opcode 0x%02X not supported in minimal build (objects/functions only)\n", code);
					break;
				}
				
				case SWF_ACTION_CALL_METHOD:
				{
					out_script << "\t" << "// CallMethod" << endl
							   << "\t" << "actionCallMethod(app_context, str_buffer);" << endl;
							
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
							   << to_string(next_str_i - 1) << ");" << endl;
							
					break;
				}
				
				// REMOVED OPCODE: StrictEquals - not supported in minimal build
				case SWF_ACTION_STRICT_EQUALS:
				// REMOVED OPCODE: Greater - not supported in minimal build
				case SWF_ACTION_GREATER:
				{
					EXC_ARG("Opcode 0x%02X not supported in minimal build (objects/functions only)\n", code);
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
					
					if (context.inside_function2)
					{
						// Inside DefineFunction2: store to local registers array
						out_script << "\t" << "peekVar(app_context, &regs[" << (int)register_num << "]);" << endl;
					}
					else
					{
						// Outside functions: store to global registers
						out_script << "\t" << "actionStoreRegister(app_context, " << (int)register_num << ");" << endl;
					}
					
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
					
					u8 register_count = VAL(u8, action_buffer);
					action_buffer += 1;
					
					u16 flags = VAL(u16, action_buffer);
					action_buffer += 2;
					
					// Parse parameters
					std::vector<std::pair<u8, std::string>> params;
					for (u16 i = 0; i < num_params; i++)
					{
						u8 reg = VAL(u8, action_buffer);
						action_buffer += 1;
						
						char* param_name = action_buffer;
						size_t param_len = strlen(param_name);
						action_buffer += param_len + 1;
						
						params.push_back(std::make_pair(reg, std::string(param_name)));
					}
					
					u16 code_size = VAL(u16, action_buffer);
					action_buffer += 2;
					
					// Generate unique function ID
					static int func2_counter = 0;
					std::string func_id = std::string("func2_") + (name_len > 0 ? std::string(func_name) : "anonymous") + "_" + std::to_string(func2_counter++);
					
				// Add function declaration to header (uses app_context)
				context.out_script_decls << endl << "ActionVar " << func_id << "(SWFAppContext* app_context, ActionVar* args, u32 arg_count, ActionVar* registers, void* this_obj);" << endl;
					// Generate function definition in out_script_defs
					context.out_script_defs << endl << endl
						<< "// DefineFunction2: " << (name_len > 0 ? func_name : "(anonymous)") << endl
						<< "ActionVar " << func_id << "(SWFAppContext* app_context, ActionVar* args, u32 arg_count, ActionVar* registers, void* this_obj)" << endl
						<< "{" << endl;
						
					// Initialize local registers
					if (register_count > 0)
					{
						context.out_script_defs << "\tActionVar regs[" << (int)register_count << "];" << endl;
						context.out_script_defs << "\tmemset(regs, 0, sizeof(regs));" << endl;
					}
					
					// Parse flags
					bool preload_this = (flags & 0x0001);
					bool preload_arguments = (flags & 0x0002);
					bool preload_super = (flags & 0x0004);
					bool preload_root = (flags & 0x0008);
					bool preload_parent = (flags & 0x0010);
					bool preload_global = (flags & 0x0020);
					bool suppress_this = (flags & 0x0080);
					bool suppress_arguments = (flags & 0x0100);
					bool suppress_super = (flags & 0x0200);
					
					// Preload special variables into registers
					int next_reg = 1; // Register 0 is reserved
					
					if (preload_this && !suppress_this)
					{
						context.out_script_defs << "\t// Preload 'this' into register " << next_reg << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_OBJECT;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = (u64)this_obj;" << endl;
						next_reg++;
					}
					
					if (preload_arguments && !suppress_arguments)
					{
						context.out_script_defs << "\t// Preload 'arguments' into register " << next_reg << endl;
						context.out_script_defs << "\t// Create arguments array object" << endl;
						context.out_script_defs << "\tASArray* arguments_array = allocArray(arg_count);" << endl;
						context.out_script_defs << "\tfor (u32 i = 0; i < arg_count; i++) {" << endl;
						context.out_script_defs << "\t\tsetArrayElement(arguments_array, i, &args[i]);" << endl;
						context.out_script_defs << "\t}" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_ARRAY;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = (u64)arguments_array;" << endl;
						next_reg++;
					}
					
					if (preload_super && !suppress_super)
					{
						context.out_script_defs << "\t// Preload 'super' into register " << next_reg << endl;
						context.out_script_defs << "\t// TODO: Create super reference (requires prototype chain support)" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_UNDEFINED;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = 0;" << endl;
						next_reg++;
					}
					
					if (preload_root)
					{
						context.out_script_defs << "\t// Preload '_root' into register " << next_reg << endl;
						context.out_script_defs << "\textern MovieClip root_movieclip;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_MOVIECLIP;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = (u64)&root_movieclip;" << endl;
						next_reg++;
					}
					
					if (preload_parent)
					{
						context.out_script_defs << "\t// Preload '_parent' into register " << next_reg << endl;
						context.out_script_defs << "\t// For now, _parent points to _root (no clip hierarchy in NO_GRAPHICS mode)" << endl;
						context.out_script_defs << "\textern MovieClip root_movieclip;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_MOVIECLIP;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = (u64)&root_movieclip;" << endl;
						next_reg++;
					}
					
					if (preload_global)
					{
						context.out_script_defs << "\t// Preload '_global' into register " << next_reg << endl;
						context.out_script_defs << "\textern ASObject* global_object;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].type = ACTION_STACK_VALUE_OBJECT;" << endl;
						context.out_script_defs << "\tregs[" << next_reg << "].data.numeric_value = (u64)global_object;" << endl;
						next_reg++;
					}
					
					// Bind parameters to registers or variables
					for (size_t i = 0; i < params.size(); i++)
					{
						if (params[i].first == 0)
						{
							// Variable parameter
							context.out_script_defs << "\tif (" << i << " < arg_count) {" << endl;
							context.out_script_defs << "\t\tsetVariableByName(\"" << params[i].second << "\", &args[" << i << "]);" << endl;
							context.out_script_defs << "\t}" << endl;
						}
						else
						{
							// Register parameter
							context.out_script_defs << "\tif (" << i << " < arg_count) {" << endl;
							context.out_script_defs << "\t\tregs[" << (int)params[i].first << "] = args[" << i << "];" << endl;
							context.out_script_defs << "\t}" << endl;
						}
					}
					
					// Parse function body recursively
					context.out_script_defs << endl << "\t// Function body (" << code_size << " bytes)" << endl;
					
					// Save the function body boundaries
					char* func_body_start = action_buffer;
					char* func_body_end = action_buffer + code_size;
					
					// Create a temporary buffer for the function body that ends with END_OF_ACTIONS
					// This ensures parseActions stops at the right place
					char* temp_buffer = (char*)malloc(code_size + 1);
					memcpy(temp_buffer, func_body_start, code_size);
					temp_buffer[code_size] = 0x00; // Add END_OF_ACTIONS marker
					
					// Set flag to indicate we're inside a DefineFunction2 (for local register handling)
					bool prev_inside_function2 = context.inside_function2;
					context.inside_function2 = true;
					
					char* temp_ptr = temp_buffer;
					parseActions(context, temp_ptr, context.out_script_defs);
					free(temp_buffer);
					
					// Restore previous state
					context.inside_function2 = prev_inside_function2;
					
					// Advance the actual buffer past the function body
					action_buffer = func_body_end;
					
					context.out_script_defs << endl << "\t// Return undefined if no explicit return" << endl;
					context.out_script_defs << "\tActionVar ret;" << endl;
					context.out_script_defs << "\tret.type = ACTION_STACK_VALUE_UNDEFINED;" << endl;
					context.out_script_defs << "\tret.data.numeric_value = 0;" << endl;
					context.out_script_defs << "\treturn ret;" << endl;
					context.out_script_defs << "}" << endl;
					
					// Generate runtime call to register function
					out_script << "\t// DefineFunction2: " << (name_len > 0 ? func_name : "(anonymous)") << endl;
					out_script << "\tactionDefineFunction2(app_context, \"" << (name_len > 0 ? func_name : "") << "\", "
							   << func_id << ", " << num_params << ", " << (int)register_count << ", " << flags << ");" << endl;
							
					// action_buffer has already been advanced by parseActions
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
					
					// Parse parameter names
					std::vector<std::string> params;
					for (u16 i = 0; i < num_params; i++)
					{
						char* param_name = action_buffer;
						size_t param_len = strlen(param_name);
						action_buffer += param_len + 1;
						params.push_back(std::string(param_name));
					}
					
					u16 code_size = VAL(u16, action_buffer);
					action_buffer += 2;
					
					// Generate unique function ID
					static int func_counter = 0;
					std::string func_id = std::string("func_") + (name_len > 0 ? std::string(func_name) : "anonymous") + "_" + std::to_string(func_counter++);
					
					// Add function declaration to header (uses app_context)
					context.out_script_decls << endl << "void " << func_id << "(SWFAppContext* app_context);" << endl;
					
					// Generate function definition
					context.out_script_defs << endl << endl
						<< "// DefineFunction: " << (name_len > 0 ? func_name : "(anonymous)") << endl
						<< "void " << func_id << "(SWFAppContext* app_context)" << endl
						<< "{" << endl;
						
					// Bind parameters (simple DefineFunction uses variables, not registers)
					for (size_t i = 0; i < params.size(); i++)
					{
						context.out_script_defs << "\t// TODO: Bind parameter '" << params[i] << "' from arguments" << endl;
					}
					
					// Parse function body recursively
					context.out_script_defs << endl << "\t// Function body (" << code_size << " bytes)" << endl;
					
					char* func_body_start = action_buffer;
					char* func_body_end = action_buffer + code_size;
					
					// Create temporary buffer with END_OF_ACTIONS marker
					char* temp_buffer = (char*)malloc(code_size + 1);
					memcpy(temp_buffer, func_body_start, code_size);
					temp_buffer[code_size] = 0x00;
					
					char* temp_ptr = temp_buffer;
					parseActions(context, temp_ptr, context.out_script_defs);
					free(temp_buffer);
					
					action_buffer = func_body_end;
					
					context.out_script_defs << "}" << endl;
					
					// Generate runtime call to register function
					out_script << "\t// DefineFunction: " << (name_len > 0 ? func_name : "(anonymous)") << endl;
					out_script << "\tactionDefineFunction(app_context, \"" << (name_len > 0 ? func_name : "") << "\", "
							   << func_id << ", " << num_params << ");" << endl;
							
					break;
				}
				
				default:
				{
					EXC_ARG("Unimplemented action 0x%02X\n", code);
					
					break;
				}
			}
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
	
	void SWFAction::declareString(Context& context, char* str)
	{
		// Check if this string was already declared (deduplication)
		auto it = string_to_id.find(str);
		if (it != string_to_id.end())
		{
			// String already exists - don't create duplicate
			return;
		}
		
		// New string - assign ID and declare
		string_to_id[str] = next_str_i;
		context.out_script_defs << endl << "char* str_" << next_str_i << " = \"" << str << "\";";
		context.out_script_decls << endl << "extern char* str_" << next_str_i << ";";
		next_str_i += 1;
	}
	
	void SWFAction::declareEmptyString(Context& context, size_t size)
	{
		context.out_script_defs << endl << "char str_" << next_str_i << "[" << to_string(size) << "];";
		context.out_script_decls << endl << "extern char str_" << next_str_i << "[];";
		next_str_i += 1;
	}
	
	size_t SWFAction::getStringId(const char* str)
	{
		auto it = string_to_id.find(str);
		if (it != string_to_id.end())
		{
			return it->second;
		}
		
		// This shouldn't happen if declareString was called first
		// Return 0 for "no ID" (dynamic strings)
		return 0;
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