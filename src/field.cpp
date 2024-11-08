#include <cstdio>
#include <exception>

#include <field.hpp>

#define VAL(type, x) ((void*) *((type*) x))

#include <cstdio>

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
				fprintf(stderr, "No type set for field.\n");
				throw std::exception();
			}
			
			case SWF_FIELD_UI8:
			{
				value = VAL(u8, field_buffer);
				field_buffer += 1;
				
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
				fprintf(stderr, "Field type %d not implemented.\n", type);
				throw std::exception();
			}
		}
		
		return field_buffer;
	}
};