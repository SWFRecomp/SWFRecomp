#include <recomp.h>
#include <constants.h>
#include <draws.h>

int main()
{
	SWFAppContext app_context;
	app_context.frame_funcs = frame_funcs;
	app_context.width = FRAME_WIDTH;
	app_context.height = FRAME_HEIGHT;
	app_context.stage_to_ndc = stage_to_ndc;
	
	app_context.shape_data = (char*) shape_data;
	app_context.shape_data_size = sizeof(shape_data);
	app_context.transform_data = (char*) transform_data;
	app_context.transform_data_size = sizeof(transform_data);
	app_context.color_data = (char*) color_data;
	app_context.color_data_size = sizeof(color_data);
	app_context.gradmat_data = (char*) gradmat_data;
	app_context.gradmat_data_size = sizeof(gradmat_data);
	
	swfStart(&app_context);
}