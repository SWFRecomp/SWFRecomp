#include <cstdio>
#include <exception>

#include <field.hpp>

#define VAL(type, x) ((u64) *((type*) x))

namespace SWFRecomp
{
	SWFField::SWFField()
	{
		
	}
	
	char* SWFField::parse(char* field_buffer)
	{
		switch (type)
		{
			case SWF_FIELD_NONE:
			{
				EXC("No type set for field.\n");
			}
			
			case SWF_FIELD_SI8:
			{
				value = VAL(s8, field_buffer);
				field_buffer += 1;
				
				break;
			}
			
			case SWF_FIELD_SI16:
			{
				value = VAL(s16, field_buffer);
				field_buffer += 2;
				
				break;
			}
			
			case SWF_FIELD_SI32:
			{
				value = VAL(s32, field_buffer);
				field_buffer += 4;
				
				break;
			}
			
			case SWF_FIELD_UI8:
			{
				value = VAL(u8, field_buffer);
				field_buffer += 1;
				
				break;
			}
			
			case SWF_FIELD_UI16:
			{
				value = VAL(u16, field_buffer);
				field_buffer += 2;
				
				break;
			}
			
			case SWF_FIELD_UI32:
			{
				value = VAL(u32, field_buffer);
				field_buffer += 4;
				
				break;
			}
			
			default:
			{
				EXC_ARG("Field type %d not implemented.\n", type);
			}
		}
		
		return field_buffer;
	}
};